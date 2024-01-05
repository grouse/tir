#include "core.h"

#if defined(_WIN32)
#include "win32_core.cpp"
#elif defined(__linux__)
#include "linux_core.cpp"
#else
#error "unsupported platform"
#endif

i32 next_type_id = 0;

bool add_log_sink(sink_proc_t *sink)
{
    if (log_sinks.count == decltype(log_sinks)::capacity()) return false;
    array_add(&log_sinks, sink);
    return true;
}

void log(const char *path, u32 line, LogType type, const char *fmt, ...)
{
    char buffer[2048];

    va_list args;
    va_start(args, fmt);

    i32 length = vsnprintf(buffer, sizeof buffer-1, fmt, args);
    buffer[MIN(length, (i32)sizeof buffer-1)] = '\0';

    va_end(args);

    for (auto sink : log_sinks) sink(path, line, type, buffer);
}

void stdio_sink(const char *path, u32 line, LogType type, const char *msg)
{
    FILE *out = stdout;
    switch (type) {
    case LOG_TYPE_INFO:
        break;
    case LOG_TYPE_ASSERT:
    case LOG_TYPE_PANIC:
    case LOG_TYPE_ERROR:
        out = stderr;
    }

    String filename = filename_of_sz(path);
    const char *type_s = sz_from_enum(type);
    fprintf(out, "%.*s:%d %s %s\n", STRFMT(filename), line, type_s, msg);
}
