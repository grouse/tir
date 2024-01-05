#ifndef THREAD_H
#define THREAD_H

#include "platform.h"

#define GUARD_MUTEX(mutex) for (i32 i_##__LINE__ = (lock_mutex(mutex), 0); i_##__LINE__ == 0; i_##__LINE__ = (unlock_mutex(mutex), 1))

struct Mutex;
struct Thread;

typedef i32 (*ThreadProc)(void *user_data);

Mutex* create_mutex();
void lock_mutex(Mutex*);
void unlock_mutex(Mutex*);

Thread* create_thread(ThreadProc proc, void *user_data = nullptr);

#endif // THREAD_H
