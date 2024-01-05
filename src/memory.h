#ifndef MEMORY_H
#define MEMORY_H

#include <new>
#include "platform.h"

extern "C" void* malloc(size_t size) NOTHROW;
extern "C" void* realloc(void* ptr, size_t size) NOTHROW;
extern "C" void free(void* ptr) NOTHROW;

extern "C" void* memcpy(void *destination, const void *source, size_t num) NOTHROW;
extern "C" void* memset(void *ptr, int value, size_t num) NOTHROW;
extern "C" int memcmp(const void *ptr1, const void *ptr2, size_t num) NOTHROW;
extern "C" void* memmove(void *destination, const void *source, size_t num) NOTHROW;

#define KiB (1024)
#define MiB (1024*1024)
#define GiB (1024LL*1024LL*1024LL)

#define M_SCRATCH_ARENAS 4
#define M_DEFAULT_ALIGN 16

enum M_Proc { M_ALLOC, M_FREE, M_REALLOC, M_RESET, };
typedef void* allocate_t(void *state, M_Proc cmd, void *old_ptr, i64 old_size, i64 size, u8 alignment);

struct Allocator {
    void *state;
    allocate_t *proc;
};

extern Allocator mem_dynamic;

#define ALLOC(alloc, size)               (alloc).proc((alloc).state, M_ALLOC,   nullptr, 0,     size, M_DEFAULT_ALIGN)
#define FREE(alloc, ptr)                 (alloc).proc((alloc).state, M_FREE,    ptr,     0,     0,    0)
#define REALLOC(alloc, ptr, osize, size) (alloc).proc((alloc).state, M_REALLOC, ptr,     osize, size, M_DEFAULT_ALIGN)
#define RESET_ALLOC(alloc)               (alloc).proc((alloc).state, M_RESET,   nullptr, 0,     0,    0)
#define RESTORE_ALLOC(alloc, ptr)        (alloc).proc((alloc).state, M_RESET,   ptr,     0,     0,    0)

#define ALLOC_A(alloc, size, alignment)  (alloc).proc((alloc).state, M_ALLOC,   nullptr, 0, size,      alignment)
#define ALLOC_T(alloc, T)           new ((alloc).proc((alloc).state, M_ALLOC,   nullptr, 0, sizeof(T), alignof(T))) T

#define ALLOC_ARR(alloc, T, count)\
    (T*)(alloc).proc(\
        (alloc).state, M_ALLOC,\
        nullptr, 0, sizeof(T)*(count),\
        alignof(T))

#define REALLOC_ARR(alloc, T, old_ptr, old_count, new_count)\
    (T*)(alloc).proc(\
        (alloc).state, M_REALLOC,\
        old_ptr, sizeof(T)*(old_count), sizeof(T)*(new_count),\
        alignof(T))

#define REALLOC_ARR_A(alloc, T, old_ptr, old_count, new_count, alignment)\
    (T*)(alloc).proc(\
        (alloc).state, M_REALLOC,\
        old_ptr, sizeof(T)*(old_count), sizeof(T)*(new_count),\
        alignment)


void init_default_allocators();

i32 get_page_size();

void* virtual_reserve(i64 size);
void* virtual_commit(void *addr, i64 size);

Allocator linear_allocator(i64 size);
Allocator malloc_allocator();

Allocator tl_linear_allocator(i64 size);

Allocator vm_freelist_allocator(i64 max_size);


struct MArena : Allocator {
    void *restore_point;
};

MArena tl_scratch_arena(Allocator conflict = {});
void release_arena(MArena *arena);
void restore_arena(MArena *arena);


struct SArena {
    MArena arena;
    SArena(MArena &&arena) : arena(arena) {}
    SArena &operator=(MArena &&arena) { this->arena = arena; return *this; }

    SArena(const MArena &arena) = delete;
    SArena(const SArena &) = delete;
    SArena& operator=(const SArena &) = delete;

    ~SArena() { release_arena(&arena); }

    MArena* operator->() { return &arena; }
    MArena& operator*() { return arena; }
    operator Allocator() { return arena; }
};

struct Mutex;

// TODO(jesper): I think I want a thread-local version of this that doesn't do
// any thread safe guarding
// It also shouldn't be in the public memory.h, just here for convenience atm because win32_memory and linux_memory shares a bunch of code. Probably there should be a vm_memory or something
struct VMFreeListState {
    struct Block {
        Block *next;
        Block *prev;
        i64 size;
    };

    u8 *mem;
    i64 reserved;
    i64 committed;

    Block *free_block;
    Mutex *mutex;

    i32 page_size;
};


#endif // MEMORY_H
