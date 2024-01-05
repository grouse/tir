#include "memory.h"
#include "core.h"
#include "thread.h"

#define M_DEBUG_VIRTUAL_HEAP_ALLOC 0

struct TlLinearAllocatorState {
    u8 *start;
    u8 *end;
    u8 *current;

    void *last;
};

struct LinearAllocatorState {
    u8 *start;
    u8 *end;
    u8 *current;

    void *last;

    Mutex *mutex;
};

Allocator mem_sys;
Allocator mem_dynamic;
thread_local MArena mem_scratch[M_SCRATCH_ARENAS];

void* align_ptr(void *ptr, u8 alignment, u8 header_size);

template<typename T>
T* get_header(void *aligned_ptr) { return (T*)((size_t)aligned_ptr-sizeof(T)); }

void init_default_allocators()
{
    PANIC_IF(mem_sys.proc != nullptr, "default allocators already initialized");

    mem_sys = malloc_allocator();
    mem_dynamic = vm_freelist_allocator(16*GiB);
}

MArena tl_scratch_arena(Allocator conflict)
{
    MArena *arena = &mem_scratch[0];
    MArena *end = mem_scratch + ARRAY_COUNT(mem_scratch);
    if (conflict.state) while (arena < end && arena->state == conflict.state) arena++;
    PANIC_IF(arena == end, "too many scratch arenas allocated");

    if (!arena->state) {
        LOG_INFO("creating scratch arena: %d", (i32)(arena - &mem_scratch[0]));
        Allocator alloc = tl_linear_allocator(1*GiB);
        arena->state = alloc.state;
        arena->proc = alloc.proc;
    }

    auto state = (TlLinearAllocatorState*)arena->state;
    arena->restore_point = state->current;
    return *arena;
}

void restore_arena(MArena *arena)
{
    //LOG_INFO("restoring arena[%d] to %p", (i32)(arena - &scratch_arenas[0]), arena->restore_point);
    arena->proc(arena->state, M_RESET, arena->restore_point, 0, 0, 0);
}

void release_arena(MArena *arena)
{
    restore_arena(arena);
    *arena = {};
}

void* align_ptr(void *ptr, u8 alignment, u8 header_size)
{
    u8 offset = header_size + alignment - (((size_t)ptr + header_size) & ((size_t)alignment-1));
    return (void*)((size_t)ptr + offset);
}

void* tl_linear_alloc(void *v_state, M_Proc cmd, void *old_ptr, i64 old_size, i64 size, u8 alignment)
{
    auto state = (TlLinearAllocatorState*)v_state;

    switch (cmd) {
    case M_ALLOC: {
        if (size == 0) return nullptr;

        u8 *ptr = (u8*)align_ptr(state->current, alignment, 0);
        PANIC_IF(ptr+size > state->end, "allocator does not have enough memory for allocation: %lld", size);

        state->current = ptr+size;
        state->last = ptr;
        return ptr;
        }
    case M_FREE:
        LOG_ERROR("free called on linear allocator, unsupported");
        return nullptr;
    case M_REALLOC: {
        if (size == 0) return nullptr;

        if (old_ptr && state->last == old_ptr) {
            state->current += size-old_size;
            return old_ptr;
        }

        void *ptr = tl_linear_alloc(v_state, M_ALLOC, nullptr, 0, size, alignment);
        if (old_size > 0) memcpy(ptr, old_ptr, old_size);
        return ptr;
        }
    case M_RESET:
        state->last = nullptr;
        state->current = old_ptr ? (u8*)old_ptr : state->start;
        return nullptr;
    }
}

void* linear_alloc(void *v_state, M_Proc cmd, void *old_ptr, i64 old_size, i64 size, u8 alignment)
{
    auto state = (LinearAllocatorState*)v_state;

    switch (cmd) {
    case M_ALLOC: {
        if (size == 0) return nullptr;

        u8 *ptr;
        GUARD_MUTEX(state->mutex) {
            ptr = (u8*)align_ptr(state->current, alignment, 0);
            PANIC_IF(ptr+size > state->end, "allocator does not have enough memory for allocation: %lld", size);

            state->current = ptr+size;
            state->last = ptr;
        }

        return ptr;
        }
    case M_FREE:
        LOG_ERROR("free called on linear allocator, unsupported");
        return nullptr;
    case M_REALLOC: {
        if (size == 0) return nullptr;

        u8 *ptr;
        GUARD_MUTEX(state->mutex) {
            if (old_ptr && state->last == old_ptr) {
                state->current += size-old_size;

                // NOTE(jesper): manual unlocking before return because the GUARD_MUTEX macro does not automatically unlock the mutex if we break out of the scope manually
                unlock_mutex(state->mutex);
                return old_ptr;
            }

            ptr = (u8*)align_ptr(state->current, alignment, 0);
            PANIC_IF(ptr+size > state->end, "allocator does not have enough memory for allocation: %lld", size);
            if (old_size > 0) memcpy(ptr, old_ptr, old_size);

            state->current = ptr+size;
            state->last = ptr;
        }

        return ptr;
        }
    case M_RESET:
        GUARD_MUTEX(state->mutex) {
            state->last = nullptr;
            state->current = old_ptr ? (u8*)old_ptr : state->start;
        }
        return nullptr;
    }
}

void* malloc_alloc(void */*v_state*/, M_Proc cmd, void *old_ptr, i64 /*old_size*/, i64 size, u8 alignment)
{
    struct Header {
        u8 offset;
        u8 alignment;
    };

    switch (cmd) {
    case M_ALLOC: {
        if (size == 0) return nullptr;

        u8 header_size = sizeof(Header);
        void *ptr = malloc(size+alignment+header_size);
        void *aligned_ptr = align_ptr(ptr, alignment, header_size);

        auto header = get_header<Header>(aligned_ptr); header->offset = (u8)((size_t)aligned_ptr - (size_t)ptr);
        header->alignment = alignment;
        return aligned_ptr;
        }
    case M_FREE:
        if (old_ptr) {
            Header *header = get_header<Header>(old_ptr);
            void *unaligned_ptr = (void*)((size_t)old_ptr - header->offset);
            free(unaligned_ptr);
        }
        return nullptr;
    case M_REALLOC: {
        void *old_unaligned_ptr = nullptr;
        if (old_ptr) {
            Header *header = get_header<Header>(old_ptr);
            ASSERT(header->alignment == alignment);
            old_unaligned_ptr = (void*)((size_t)old_ptr - header->offset);
        }

        u8 header_size = sizeof(Header);
        void *ptr = realloc(old_unaligned_ptr, size+alignment+header_size);
        void *aligned_ptr = align_ptr(ptr, alignment, header_size);

        auto header = get_header<Header>(aligned_ptr);
        header->offset = (u8)((size_t)aligned_ptr - (size_t)ptr);
        header->alignment = alignment;

        return aligned_ptr;
        }
    case M_RESET:
        LOG_ERROR("reset called on malloc allocator, unsupported");
        return nullptr;
    }
}

Allocator tl_linear_allocator(i64 size)
{
    u8 *mem = (u8*)malloc(size);

    TlLinearAllocatorState *state = (TlLinearAllocatorState*)mem;
    *state = {
        .start = mem + sizeof *state,
        .end = mem+size,
        .current = mem + sizeof *state,
        .last = nullptr,
    };

    return Allocator{ state, tl_linear_alloc };
}

Allocator linear_allocator(i64 size)
{
    u8 *mem = (u8*)malloc(size);

    LinearAllocatorState *state = new (mem) LinearAllocatorState {
        .start = mem + sizeof *state,
        .end = mem+size,
        .current = mem + sizeof *state,
        .last = nullptr,
        .mutex = create_mutex(),
    };

    return Allocator{ state, linear_alloc };
}

Allocator malloc_allocator()
{
    return Allocator{ nullptr, malloc_alloc };
}


void* vm_freelist_alloc(void *v_state, M_Proc cmd, void *old_ptr, i64 old_size, i64 size, u8 alignment)
{
    struct Header {
        u64 total_size;
        u8 offset;
        u8 alignment;
    };

    using Block = VMFreeListState::Block;

    auto state = (VMFreeListState*)v_state;

    switch (cmd) {
    case M_ALLOC: {
            if (size == 0) return nullptr;

            lock_mutex(state->mutex);
            defer { unlock_mutex(state->mutex); };

            u8 header_size = sizeof(Header);
            i64 minimum_size = sizeof(Block);
            i64 required_size = size+alignment+header_size;

            auto block = state->free_block;
            while (block && block->size < required_size) block = block->next;
            if (!block || block->size - required_size < minimum_size) {
                i64 commit_size = ROUND_TO(required_size, state->page_size);
                if (state->committed + commit_size > state->reserved) {
                    LOG_ERROR("virtual size exceeded");
                    return nullptr;
                }

                block = (Block*)virtual_commit(state->mem+state->committed, commit_size);
                block->next = state->free_block;
                block->prev = nullptr;
                block->size = commit_size;

                if (state->free_block) state->free_block->prev = block;
                state->free_block = block;

                state->committed += commit_size;

#if M_DEBUG_VIRTUAL_HEAP_ALLOC
                LOG_INFO("commmitted new block [%p] of size %lld, total committed: %lld", block, commit_size, state->committed);
#endif
            }

            void *ptr;
            i64 total_size = required_size;
            if (block->size-required_size < minimum_size) {
#if M_DEBUG_VIRTUAL_HEAP_ALLOC
                LOG_INFO("popping block [%p] of size %d", block, block->size);
#endif
                if (state->free_block == block) {
                    state->free_block = block->next ? block->next : block->prev;
                }

                if (block->next->prev) block->next->prev = block->prev;
                if (block->prev) block->prev->next = block->next;
                total_size = block->size;
                ptr = block;
            } else {
                ptr = (u8*)block + block->size - required_size;
                block->size -= required_size;
#if M_DEBUG_VIRTUAL_HEAP_ALLOC
                LOG_INFO("shrinking block [%p] by %d bytes, %lld remain", block, required_size, block->size);
#endif
            }

            void *aligned_ptr = align_ptr(ptr, alignment, header_size);

            Header* header = get_header<Header>(aligned_ptr);
            header->offset = (u8)((size_t)aligned_ptr - (size_t)ptr);
            header->alignment = alignment;
            header->total_size = total_size;

#if M_DEBUG_VIRTUAL_HEAP_ALLOC
            for (Block *b = state->free_block; b; b = b->next) {
                LOG_RAW("\t\t[%p] size: %lld, prev [%p], next [%p]\n", b, b->size, b->prev, b->next);
            }

            LOG_INFO("allocated [%p]", aligned_ptr);
#endif

            return aligned_ptr;
        } break;
    case M_FREE: {
            if (!old_ptr) break;

            lock_mutex(state->mutex);
            defer { unlock_mutex(state->mutex); };

            // TODO(jesper): do we try to expand neighbor free blocks to reduce fragmentation?
            Header* header = get_header<Header>(old_ptr);
            void *unaligned_ptr = (void*)((size_t)old_ptr - header->offset);
            i64 total_size = header->total_size;

            auto block = (Block*)unaligned_ptr;
            block->size = total_size;
            block->next = state->free_block;
            block->prev = nullptr;

            if (state->free_block) state->free_block->prev = block;
            state->free_block = block;

#if M_DEBUG_VIRTUAL_HEAP_ALLOC
            LOG_INFO("inserting free block [%p] size %lld", block, block->size);
            for (Block *b = state->free_block; b; b = b->next) {
                LOG_RAW("\t\t[%p] size: %lld, prev [%p], next [%p]\n", b, b->size, b->prev, b->next);
            }
            LOG_INFO("freed [%p]", old_ptr);
#endif

        } break;
    case M_REALLOC: {
            if (size == 0) return nullptr;

            // TODO(jesper): if size < old_size, shrink the allocation and push the free block to the list
            // TODO(jesper): we should check if the current allocation can be expanded
            void *nptr = vm_freelist_alloc(v_state, M_ALLOC, nullptr, 0, size, alignment);
            if (old_ptr == nullptr) return nptr;

            memcpy(nptr, old_ptr, old_size);
            vm_freelist_alloc(v_state, M_FREE, old_ptr, 0, 0, 0);
            return nptr;
        } break;
    case M_RESET:
        LOG_ERROR("unsupported command called for vm_freelist_allocator: reset");
        break;
    }

    return nullptr;
}
