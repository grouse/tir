#include "thread.h"
#include "core.h"

#include <pthread.h>
#include <errno.h>

struct Thread {
	pthread_t handle;
	ThreadProc user_proc;
	void *user_data;
};

Mutex* create_mutex()
{
    extern Allocator mem_sys;

	pthread_mutex_t *mutex = ALLOC_T(mem_sys, pthread_mutex_t);

	int r = pthread_mutex_init(mutex, nullptr);
	PANIC_IF(r != 0, "failed to create mutex, errno: %d", errno);

	return (Mutex*)mutex;
}

void lock_mutex(Mutex *m)
{
	pthread_mutex_t *mutex = (pthread_mutex_t*)m;
	int r = pthread_mutex_lock(mutex);
	PANIC_IF(r != 0, "failed to lock mutex, errno: %d", errno);
}

void unlock_mutex(Mutex *m)
{
	pthread_mutex_t *mutex = (pthread_mutex_t*)m;
	int r = pthread_mutex_unlock(mutex);
	PANIC_IF(r != 0, "failed to lock mutex, errno: %d", errno);
}

Thread* create_thread(ThreadProc proc, void *user_data)
{
    extern Allocator mem_sys;

	i32 result;

	pthread_attr_t attr;
	result = pthread_attr_init(&attr);
	PANIC_IF(result != 0, "failed initialising thread attributes");

	Thread *thread = ALLOC_T(mem_sys, Thread) {
        .user_proc = proc,
        .user_data = user_data,
    };

	result = pthread_create(&thread->handle, &attr,
	[](void *data) -> void*
	{
		Thread *thread = (Thread*)data;
		thread->user_proc(thread->user_data);
		return nullptr;
	}, thread);

	PANIC_IF(result != 0, "failed creating thread");
	return thread;
}
