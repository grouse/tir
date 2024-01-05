#include "core.h"
#include "window.h"
#include "array.h"

#include "gen/string.h"

#include <ctime>
#include <stdarg.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <X11/cursorfont.h>
#include <X11/Xlib.h>

extern void stdio_sink(const char *path, u32 line, LogType type, const char *msg);
FixedArray<sink_proc_t*, 10> log_sinks{ stdio_sink };

MouseCursor current_cursor;
Cursor cursors[MC_MAX];

void push_cursor(MouseCursor c)
{
    extern MouseCursor current_cursor;
    current_cursor = c;
}

bool debugger_attached()
{
    return false;
}

timespec get_timespec(clockid_t clock)
{
    timespec ts;
    clock_gettime(clock, &ts);
    return ts;
}

u64 wall_timestamp()
{
    timespec ts = get_timespec(CLOCK_MONOTONIC);
	return (u64)ts.tv_sec*1000000000 + (u64)ts.tv_nsec;
}

f32 app_time_s()
{
    static timespec start_time = get_timespec(CLOCK_MONOTONIC);
    timespec now = get_timespec(CLOCK_MONOTONIC);
    timespec delta = { now.tv_sec - start_time.tv_sec, now.tv_nsec - start_time.tv_nsec };
    return (f32)delta.tv_sec + (f32)delta.tv_nsec/1000000000;
}

f32 wall_duration_s(u64 start, u64 end)
{
    return (f32)(end-start)/1000000000;
}
