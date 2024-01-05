#include "platform.h"
#include "memory.h"
#include "core.h"
#include "thread.h"

#include "win32_lite.h"

void* virtual_commit(void *addr, i64 size)
{
	void *ptr = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
	return ptr;
}

Allocator vm_freelist_allocator(i64 max_size)
{
    extern void* vm_freelist_alloc(
        void *v_state,
        M_Proc cmd,
        void *old_ptr,
        i64 old_size,
        i64 size,
        u8 alignment);

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    i64 reserve_size = ROUND_TO(max_size, si.dwPageSize);
    void *mem = VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_READWRITE);

    VMFreeListState *state = (VMFreeListState *)malloc(sizeof *state);
    state->mem = (u8*)mem;
    state->reserved = reserve_size;
    state->committed = 0;
    state->mutex = create_mutex();
    state->free_block = nullptr;
    state->page_size = si.dwPageSize;

    return Allocator{ state, vm_freelist_alloc };
}
