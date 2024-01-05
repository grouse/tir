#include "MurmurHash/MurmurHash3.cpp"

#include "core.h"

#include "array.h"
#include "string.h"
#include "memory.h"

#include "gen/string.h"

#include <unistd.h>

extern int app_main(Array<String> args);
extern Allocator mem_frame;

String exe_path;

int main(int argc, char **argv)
{
    init_default_allocators();
    mem_frame = linear_allocator(10*MiB);

	char *p = last_of(argv[0], '/');
	if (argv[0][0] == '/') {
		exe_path = { argv[0], (i32)(p-argv[0]) };
	} else {
		char buffer[PATH_MAX];
		char *wd = getcwd(buffer, sizeof buffer);
		PANIC_IF(wd == nullptr, "current working dir exceeds PATH_MAX");

		exe_path = join_path(
			{ wd, (i32)strlen(wd) },
			{ argv[0], (i32)(p-argv[0]) },
			mem_dynamic);
	}

	Array<String> args{};
	if (argc > 1) {
		array_create(&args, argc-1);
		for (i32 i = 1; i < argc; i++) {
			args[i-1] = String{ argv[i], (i32)strlen(argv[i]) };
		}
	}

    return app_main(args);
}
