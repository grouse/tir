#include "process.h"
#include "core.h"

#include <spawn.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>

struct Process {
    int pid;
};

Process* create_process(
    String exe,
    Array<String> args,
    ProcessOpts /*opts*/)
{
    SArena scratch = tl_scratch_arena();

    char *sz_exe = sz_string(exe, scratch);

    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        PANIC("posix_spawn_file_actions_init: %s", strerror(errno));
        return nullptr;
    }

    posix_spawnattr_t attr;
    if (posix_spawnattr_init(&attr) != 0) {
        PANIC("posix_spawnattr_init: %s", strerror(errno));
        return nullptr;
    }

    Array<char*> argv; array_create(&argv, args.count+2, scratch);
    argv[0] = sz_exe;
    for (auto it : iterator(args)) argv[it.index+1] = sz_string(it, scratch);
    *array_tail(argv) = nullptr;

    LOG_INFO("spawning '%s'", sz_exe);
    //for (auto it : iterator(argv)) LOG_INFO("  argv[%d] = '%s'", it.index, argv[it.index]);

    int pid;
    int result = posix_spawnp(&pid, sz_exe, &actions, &attr, argv.data, NULL);
    if (result != 0) {
        LOG_ERROR("posix_spawn exe: '%s': %s", sz_exe, strerror(errno));
    }

    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);

    return ALLOC_T(mem_dynamic, Process) {
        .pid = pid,
    };
}

void release_process(Process *p)
{
    wait_for_process(p);
    FREE(mem_dynamic, p);
}

bool wait_for_process(Process *p, int *exit_code)
{
    int status;
    do {
        if (waitpid(p->pid, &status, WUNTRACED | WCONTINUED) == -1) {
            PANIC("waitpid: %s", strerror(errno));
            return false;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    if (exit_code) *exit_code = WEXITSTATUS(status);
    return true;
}


bool get_exit_code(Process */*process*/, int * /*exit_code*/)
{
	LOG_ERROR("unimplemented");
	return false;
}
