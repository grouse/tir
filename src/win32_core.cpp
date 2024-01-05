#include "core.h"
#include "array.h"

#include "win32_core.h"
#include "win32_user32.h"


#include "gen/string.h"


#include <stdarg.h>
#include <stdio.h>

extern void stdio_sink(const char *path, u32 line, LogType type, const char *msg);
extern void win32_debugger_sink(const char *path, u32 line, LogType type, const char *msg);
FixedArray<sink_proc_t*, 10> log_sinks{ stdio_sink, win32_debugger_sink };

bool debugger_attached()
{
    return IsDebuggerPresent();
}

void win32_debugger_sink(const char *path, u32 line, LogType type, const char *msg)
{
    char buffer[2048];

    String filename = filename_of_sz(path);
    const char *type_s = sz_from_enum(type);
    String s = stringf(buffer, sizeof buffer, "%.*s:%d %s %s\n\0", STRFMT(filename), line, type_s, msg);
    OutputDebugStringA(s.data);
}


u64 time_counter_frequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

u64 wall_time_counter()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return time.QuadPart;
}

f32 app_time_s()
{
    static u64 start_time = wall_time_counter();
    static u64 frequency = time_counter_frequency();

    u64 time = wall_time_counter();
    return (f64)(time - start_time) / frequency;
}

f32 wall_duration_s(u64 start, u64 end)
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (f32)(end-start) / frequency.QuadPart;
}

void set_clipboard_data(String str)
{
    if (!OpenClipboard(NULL)) return;
    defer { CloseClipboard(); };

    if (!EmptyClipboard()) return;

    i32 utf16_len = utf16_length(str);
    HANDLE clip_mem = GlobalAlloc(GMEM_MOVEABLE, (utf16_len+1) * sizeof(u16));
    defer { GlobalUnlock(clip_mem); };

    u16 *clip_str = (u16*)GlobalLock(clip_mem);
    utf16_from_string(clip_str, utf16_len, str);
    clip_str[utf16_len] = '\0';

    if (!SetClipboardData(CF_UNICODETEXT, clip_mem)) return;
}

String read_clipboard_str(Allocator mem)
{
    if (!OpenClipboard(NULL)) return {};
    defer { CloseClipboard(); };

    HANDLE clip = GetClipboardData(CF_UNICODETEXT);
    if (!clip) return {};

    wchar_t *ptr = (wchar_t*)GlobalLock(clip);
    if (!ptr) return {};
    defer { GlobalUnlock(ptr); };

    String s{};
    s.length = utf8_length((u16*)ptr, wcslen(ptr));
    s.data = ALLOC_ARR(mem, char, s.length);
    utf8_from_utf16((u8*)s.data, s.length, (u16*)ptr, wcslen(ptr));
    return s;
}

char* win32_system_error_message(DWORD error)
{
    static char buffer[2048];
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        error,
        0,
        buffer,
        sizeof buffer,
        NULL);

    return buffer;
}

Array<String> win32_commandline_args(LPWSTR pCmdLine, Allocator mem)
{
    Array<String> args{};

    // NOTE(jesper): CommandLineToArgvW sets the first argument string to the executable
    // path if and only if pCmdLine is empty
    if (pCmdLine && *pCmdLine != '\0') {
        i32 argv = 0;
        LPWSTR* argc = CommandLineToArgvW(pCmdLine, &argv);

        if (argv > 0) {
            i32 total_utf8_length = 0;
            for (i32 i = 0; i < argv; i++) {
                total_utf8_length += utf8_length(argc[i], wcslen(argc[i]));
            }

            args.data = ALLOC_ARR(mem, String, argv);
            args.count = argv;

            char *args_mem = ALLOC_ARR(mem, char, total_utf8_length);

            char *p = args_mem;
            for (i32 i = 0; i < argv; i++) {
                i32 l = wcslen(argc[i]);

                args[i].data = p;
                args[i].length = utf8_from_utf16((u8*)p, l, argc[i], l);
                p += l;
            }
        }
    }

    return args;
}

u64 wall_timestamp()
{
    FILETIME time;
    GetSystemTimeAsFileTime(&time);

    return time.dwLowDateTime | ((u64)time.dwHighDateTime << 32);
}
