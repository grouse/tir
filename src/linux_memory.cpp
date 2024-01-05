#include "memory.h"
#include "core.h"
#include "thread.h"

#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

void* virtual_commit(void *addr, i64 size)
{
	(void)size;
	return addr;
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

	int page_size = getpagesize();
    i64 reserve_size = ROUND_TO(max_size, page_size);
    void *mem = mmap(
        nullptr, reserve_size,
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,
        0, 0);
    PANIC_IF(mem == MAP_FAILED, "failed to mmap, errno: %d", errno);

    VMFreeListState *state = (VMFreeListState *)malloc(sizeof *state);
    *state = {
        .mem = (u8*)mem,
        .reserved = reserve_size,
        .committed = 0,
        .free_block = nullptr,
        .mutex = create_mutex(),
        .page_size = page_size,
    };

    return Allocator{ state, vm_freelist_alloc };
}
