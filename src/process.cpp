#include "process.h"

#if defined(__linux__)
#include "linux_process.cpp"
#elif defined(_WIN32)
#include "win32_process.cpp"
#endif

int run_process(String exe, char *argv[], ProcessOpts opts)
{
    Process *p = create_process(exe, argv, opts);
    if (!p) return false;

    int exit_code = 0;
    if (!wait_for_process(p, &exit_code)) return false;
    return exit_code;
}
