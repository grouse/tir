#include "file.h"
#include "core.h"

#include <sys/inotify.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>

#include "gen/string.h"

extern "C" char *realpath(const char *path, char *resolved_path);


bool file_exists_sz(const char *sz_path)
{
    return access(sz_path, F_OK) == 0;
}

bool file_exists(String path)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);
    return file_exists_sz(sz_path);
}

FileInfo read_file(String path, Allocator mem, i32 /*retry_count*/)
{
    FileInfo fi{};

    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);

    struct stat st;
    i32 result = stat(sz_path, &st);
    if (result != 0) {
        LOG_ERROR("couldn't stat file: %s", sz_path);
        return fi;
    }

    fi.data = (u8*)ALLOC(mem, st.st_size);

    i32 fd = open(sz_path, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("unable to open file descriptor for file: '%s' - '%s'", sz_path, strerror(errno));
    }
    ASSERT(fd >= 0);

    // TODO(jesper): we should probably error on 4gb files here and have a different API
    // for loading large files. This API is really only suitable for small files
    i64 bytes_read = read(fd, fi.data, st.st_size);
    ASSERT(bytes_read == st.st_size);
    PANIC_IF(st.st_size > i32_MAX, "file size is too large");
    fi.size = (i32)st.st_size;

    return fi;
}

void list_files(DynamicArray<String> *dst, String dir, String ext, Allocator mem, u32 flags)
{
    SArena scratch = tl_scratch_arena(mem);
    if (flags & FILE_LIST_ABSOLUTE) LOG_ERROR("unimplemented");

	DynamicArray<char*> folders{ .alloc = scratch };
	array_add(&folders, sz_string(dir, scratch));

	for (i32 i = 0; i < folders.count; i++) {
		char *sz_path = folders[i];
    	DIR *d = opendir(sz_path);

    	if (!d) {
        	LOG_ERROR("unable to open directory: %s", sz_path);
        	continue;
    	}

    	struct dirent *it;
    	while ((it = readdir(d)) != nullptr) {
        	if (it->d_name[0] == '.') continue;

			switch (it->d_type) {
        	case DT_REG: {
                if (ext && !ends_with(string(it->d_name), ext)) continue;

            	char *fp = join_path(sz_path, it->d_name, mem);
            	array_add(dst, string(fp));
				} break;
        	case DT_DIR:
        	    if (flags & FILE_LIST_RECURSIVE) {
                	char *fp = join_path(sz_path, it->d_name, mem);
                	array_add(&folders, fp);
            	} break;
			case DT_UNKNOWN:
				LOG_ERROR("unknown d_type value from readdir, supposed to fall back to stat");
				break;
			default:
				LOG_INFO("unhandled d_type for %s", it->d_name);
				break;
			}
    	}

    	closedir(d);
	}
}

void list_folders(DynamicArray<String> *dst, String dir, Allocator mem, u32 flags)
{
    SArena scratch = tl_scratch_arena(mem);
    if (flags & FILE_LIST_ABSOLUTE) LOG_ERROR("unimplemented");

	DynamicArray<char*> folders{ .alloc = scratch };
	array_add(&folders, sz_string(dir, scratch));

	for (i32 i = 0; i < folders.count; i++) {
		char *sz_path = folders[i];
    	DIR *d = opendir(sz_path);

    	if (!d) {
        	LOG_ERROR("unable to open directory: %s", sz_path);
        	continue;
    	}

    	struct dirent *it;
    	while ((it = readdir(d)) != nullptr) {
        	if (it->d_name[0] == '.') continue;

			switch (it->d_type) {
			case DT_REG: break;
			case DT_DIR: {
            	char *fp = join_path(sz_path, it->d_name, mem);
            	array_add(dst, String{ fp, (i32)strlen(fp) });
        	    if (flags & FILE_LIST_RECURSIVE) {
                	char *fp = join_path(sz_path, it->d_name, mem);
                	array_add(&folders, fp);
            	}
				} break;
			case DT_UNKNOWN:
				LOG_ERROR("unknown d_type value from readdir, supposed to fall back to stat");
				break;
			default:
				LOG_INFO("unhandled d_type for %s", it->d_name);
				break;
			}
    	}

    	closedir(d);
	}
}

void write_file(String path, StringBuilder *sb)
{
    SArena scratch = tl_scratch_arena(sb->alloc);
    char *sz_path = sz_string(path, scratch);
    i32 fd = open(sz_path, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    if (fd < 0) {
        LOG_ERROR("unable to open file descriptor for file: '%s' - '%s'", sz_path, strerror(errno));
        return;
    }

    StringBuilder::Block *it = &sb->head;
    while (it && it->written > 0) {
        write(fd, it->data, it->written);
        it = it->next;
    }

    close(fd);
}

void remove_file(String path)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);
    int result = remove(sz_path);
    if (result != 0) LOG_ERROR("error removing file: '%s'", sz_path);
}

String absolute_path(String relative, Allocator mem)
{
    SArena scratch = tl_scratch_arena(mem);
    char *sz_path = sz_string(relative, scratch);

	if (sz_path[0] != '/') {
        char buffer[PATH_MAX];
        char *wd = getcwd(buffer, sizeof buffer);
        PANIC_IF(wd == nullptr, "current working dir exceeds PATH_MAX");

        sz_path = join_path(wd, sz_path, scratch);
    }

    char *rl_path = realpath(sz_path, nullptr);
    if (!rl_path) return {};

    String r = create_string(rl_path, mem);
    free(rl_path);
    return r;
}

bool is_directory(String path)
{
    SArena scratch = tl_scratch_arena();
	char *sz_path = sz_string(path, scratch);

	struct stat st;
	if (stat(sz_path, &st)) {
		LOG_ERROR("failed to stat file: '%s', errno: %d", sz_path, errno);
		return false;
	}

	return st.st_mode & S_IFDIR;
}

String get_exe_folder(Allocator mem)
{
    extern String exe_path;
    return duplicate_string(exe_path, mem);
}

String get_working_dir(Allocator mem)
{
	char buffer[PATH_MAX];
	char *wd = getcwd(buffer, sizeof buffer);
	PANIC_IF(wd == nullptr, "current working dir exceeds PATH_MAX");

	return create_string(wd, (i32)strlen(wd), mem);
}

void set_working_dir(String path)
{
    SArena scratch = tl_scratch_arena();
	char *sz_path = sz_string(path, scratch);

	if (chdir(sz_path)) {
		LOG_ERROR("failed to set working dir to: '%s', errno: %d", sz_path, errno);
	}
}

FileHandle open_file(String path, FileOpenMode mode)
{
    SArena scratch = tl_scratch_arena();
	char *sz_path = sz_string(path, scratch);

	int flags = O_RDWR;
	if (mode == FILE_OPEN_CREATE) flags |= O_CREAT;
	if (mode == FILE_OPEN_TRUNCATE) flags |= O_TRUNC;

	int fd = open(sz_path, flags);

	if (fd == -1) {
		LOG_ERROR("unhandled error opening file '%s', errono: %d: '%s'", sz_path, errno, strerror(errno));
		return nullptr;
	}

	return (FileHandle)(i64)fd;
}

void write_file(FileHandle handle, char *data, i32 bytes)
{
	int fd = (int)(i64)handle;
	ASSERT(fd != -1);

	ssize_t res = write(fd, data, bytes);
	if (res == -1) {
		LOG_ERROR("unhandled write error %d: '%s'", errno, strerror(errno));
	}
}

void close_file(FileHandle handle)
{
	int fd = (int)(i64)handle;
	close(fd);
}

String select_folder_dialog(Allocator /*mem*/)
{
	LOG_ERROR("unimplemented");
	return {};
}

void create_filewatch(String folder, DynamicArray<FileEvent> *events, Mutex *events_mutex)
{
    SArena scratch = tl_scratch_arena();
	Array<String> folders = list_folders(folder, scratch, FILE_LIST_RECURSIVE);

    struct FileHandlePair {
        int fd;
        String path;
    };

    struct FileWatchData {
        int fd;

        Array<String> folders;
		Array<int> fds;

        Mutex *events_mutex;
        DynamicArray<FileEvent> *events;
    };

    FileWatchData *fwd = ALLOC_T(mem_dynamic, FileWatchData) {
        .fd = inotify_init(),
        .events_mutex = events_mutex,
        .events = events,
    };
    PANIC_IF(fwd->fd == -1, "failed initialising inotify: '%s'", strerror(errno));

    array_create(&fwd->folders, folders.count, mem_dynamic);
    array_create(&fwd->fds, folders.count, mem_dynamic);

    for (i32 i = 0; i < folders.count; i++) {
		fwd->folders[i] = absolute_path(folders[i], mem_dynamic);
        fwd->fds[i] = inotify_add_watch(
			fwd->fd,
			sz_string(folders[i], scratch),
			IN_CLOSE_WRITE|IN_CREATE|IN_DELETE|IN_DELETE_SELF|IN_MOVE);
        PANIC_IF(fwd->fds[i] == -1, "failed adding watch: '%s'", strerror(errno));
    }

    create_thread([](void *data) -> i32
    {
        FileWatchData *fwd = (FileWatchData*)data;

        do {
            char inotify_buffer[sizeof(inotify_event)+NAME_MAX+1];
            i32 bytes = read(fwd->fd, inotify_buffer, sizeof inotify_buffer);
            PANIC_IF(bytes <= 0, "unexpected negative/0 number of bytes read from inotify");

            i32 i = 0;
            while (i < bytes) {
                inotify_event *event = (inotify_event*)(inotify_buffer+i);
                i += sizeof *event + event->len;

                String folder = "";
                for (i32 i = 0; i < fwd->fds.count; i++) {
                    if (fwd->fds[i] == event->wd) {
                        folder = fwd->folders[i];
                        break;
                    }
                }

                String name { event->name, (i32)strlen(event->name) };

                if (false) {
                    LOG_INFO("inotify event: '%.*s' for watch '%.*s'", STRFMT(name), STRFMT(folder));
                    if (event->mask & IN_ACCESS) LOG_INFO("\taccessed");
                    if (event->mask & IN_ATTRIB) LOG_INFO("\tmeta data changed");
                    if (event->mask & IN_CLOSE_WRITE) LOG_INFO("\tclose write");
                    if (event->mask & IN_CLOSE_NOWRITE) LOG_INFO("\tclose nowrite");
                    if (event->mask & IN_CREATE) LOG_INFO("\tcreated");
                    if (event->mask & IN_DELETE) LOG_INFO("\tdeleted");
                    if (event->mask & IN_DELETE_SELF) LOG_INFO("\twatched file/dir deleted");
                    if (event->mask & IN_MODIFY) LOG_INFO("\tmodified");
                    if (event->mask & IN_MOVE_SELF) LOG_INFO("\twatched file/dir moved");
                    if (event->mask & IN_MOVED_FROM) LOG_INFO("\tmoved from");
                    if (event->mask & IN_MOVED_TO) LOG_INFO("\tmoved to");
                    if (event->mask & IN_OPEN) LOG_INFO("\topened");
                }

                FileEvent fe{};
                if (event->mask & IN_CLOSE_WRITE) fe.type = FE_MODIFY;
                else if (event->mask & IN_CREATE) fe.type = FE_CREATE;
                else if (event->mask & IN_DELETE) fe.type = FE_DELETE;
                else if (event->mask & IN_DELETE_SELF) fe.type = FE_DELETE;
                else if (event->mask & IN_MODIFY) fe.type = FE_MODIFY;
                else if (event->mask & IN_MOVED_FROM) fe.type = FE_DELETE;
                else if (event->mask & IN_MOVED_TO) fe.type = FE_CREATE;

                if (fe.type == FE_UNKNOWN) {
                    LOG_ERROR("unknown file event");
                    continue;
                }

                fe.path = join_path(folder, name, mem_dynamic);
                GUARD_MUTEX(fwd->events_mutex) array_add(fwd->events, fe);
            }
        } while (true);
    }, fwd);
}

u64 file_modified_timestamp(String path)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);

    struct stat st;
    i32 result = stat(sz_path, &st);

    if (result != 0) {
        LOG_ERROR("couldn't stat file: %s", sz_path);
        return -1;
    }

    return st.st_mtim.tv_sec;
}
