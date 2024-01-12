#ifndef PROCESS_H
#define PROCESS_H

#include "string.h"

struct Process;
typedef void (*StdOutProc) (String str);

struct ProcessOpts {
    StdOutProc stdout;
};


Process* create_process(String exe, char *argv[], ProcessOpts opts = {});
void release_process(Process *process);

int run_process(String exe, char *argv[], ProcessOpts opts = {});
bool wait_for_process(Process *process, int *exit_code = nullptr);
bool get_exit_code(Process *process, int *exit_code = nullptr);

#endif // PROCESS_H
