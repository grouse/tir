#include "process.h"
#include "thread.h"

#include "win32_core.h"

struct Process {
    HANDLE process_handle;

    HANDLE stdout_rd;
    Thread *stdout_thread;

    StdOutProc stdout_proc;
};

Process* create_process(String exe, Array<String> args, ProcessOpts opts)
{
    SArena scratch = tl_scratch_arena();
    Process *p = ALLOC_T(mem_dynamic, Process);

    StringBuilder sb{ .alloc = scratch };
    if (extension_of(exe) == ".bat") {
        append_string(&sb, "/c");
        exe = "c:\\windows\\system32\\cmd.exe";
    }

    for (i32 i = 0; i < args.count; i++) {
        append_string(&sb, args[i]);
        if (i < args.count-1) append_string(&sb, " ");
    }

    String s_args = create_string(&sb, scratch);
    wchar_t *wsz_exe = wsz_string(exe, scratch);
    wchar_t *wsz_args = wsz_string(s_args, scratch);

    HANDLE stdout_wr = NULL;
    defer { if (stdout_wr) CloseHandle(stdout_wr); };

    if (opts.stdout) {
        SECURITY_ATTRIBUTES sa{
            .nLength = sizeof sa,
            .bInheritHandle = false,
        };

        if (!CreatePipe(&p->stdout_rd, &stdout_wr, &sa, 0)) {
            LOG_ERROR("unable to create stdout pipes: (%d) %s", WIN32_ERR_STR);
            FREE(mem_dynamic, p);
            return nullptr;
        }

        if (!SetHandleInformation(stdout_wr, HANDLE_FLAG_INHERIT, 1)) {
            LOG_ERROR("unable to set inherit flag on stdout_rd handle: (%d) %s", WIN32_ERR_STR);
            CloseHandle(p->stdout_rd);
            FREE(mem_dynamic, p);
            return nullptr;
        }
    }

    STARTUPINFOW si{
        .cb = sizeof si,
        .dwFlags = opts.stdout ? STARTF_USESTDHANDLES : 0u,
        .hStdOutput = opts.stdout ? stdout_wr : NULL,
        .hStdError = opts.stdout ? stdout_wr : NULL,
    };

    DWORD flags = CREATE_NO_WINDOW;

    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(wsz_exe, wsz_args, NULL, NULL, true, flags, NULL, NULL, &si, &pi)) {
        CloseHandle(p->stdout_rd);
        LOG_ERROR("unable to create process: '%S' -- '%S': (%d) %s", wsz_exe, wsz_args, WIN32_ERR_STR);
        FREE(mem_dynamic, p);
        return nullptr;
    }

    p->process_handle = pi.hProcess;
    p->stdout_proc = opts.stdout;

    if (p->stdout_proc) {
        p->stdout_thread = create_thread(
            [](void *thread_data) -> i32
            {
                Process *p = (Process*)thread_data;

                char buffer[255];
                while (true) {
                    DWORD bytes_read;
                    if (!ReadFile(p->stdout_rd, buffer, sizeof buffer, &bytes_read, NULL) || bytes_read == 0) {
                        break;
                    }

                    p->stdout_proc(String{ buffer, (i32)bytes_read });
                }

                return 0;
            }, p);
    }

    return p;
}

void release_process(Process *process)
{
    if (process) FREE(mem_dynamic, process);
}

bool wait_for_process(Process */*process*/, int */*exit_code*/)
{
    LOG_ERROR("unimplemented");
    return false;
}

bool get_exit_code(Process *process, i32 *exit_code)
{
    DWORD dw_exit_code;
    if (process && GetExitCodeProcess(process->process_handle, &dw_exit_code)) {
        LOG_INFO("exit code: %d", dw_exit_code);
        if (exit_code) *exit_code = dw_exit_code;
        return true;
    }

    return false;
}
