#ifndef PROCESS_H
#define PROCESS_H

#include "string.h"
#include "array.h"

#include <initializer_list>

struct Process;
typedef void (*StdOutProc) (String str);

struct ProcessOpts {
    StdOutProc stdout;
};

int run_process(String exe, Array<String> args, ProcessOpts opts = {});

Process* create_process(String exe, Array<String> args, ProcessOpts opts = {});
void release_process(Process *process);

bool wait_for_process(Process *process, int *exit_code = nullptr);
bool get_exit_code(Process *process, int *exit_code = nullptr);


// initializer_list utils
inline Process* create_process(String exe, std::initializer_list<String> args, ProcessOpts opts = {})
{
    return create_process(exe, Array<String>{ (String*)args.begin(), (i32)args.size() }, opts);
}

inline int run_process(String exe, std::initializer_list<String> args, ProcessOpts opts = {})
{
    return run_process(exe, Array<String>{ (String*)args.begin(), (i32)args.size() }, opts);
}

#endif // PROCESS_H
