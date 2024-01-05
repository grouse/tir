#ifndef FILE_H
#define FILE_H

#ifdef _WIN32
#include "win32_lite.h"
using FileHandle = HANDLE;
#else
struct FileHandle_;
using FileHandle = FileHandle_*;
#endif

#include "array.h"
#include "string.h"
#include "thread.h"

struct FileInfo {
    u8 *data;
    i32 size;
};

enum FileOpenMode {
    FILE_OPEN_CREATE = 1,
    FILE_OPEN_TRUNCATE,
};

enum ListFileFlags : u32 {
    FILE_LIST_RECURSIVE = 1 << 0,
    FILE_LIST_ABSOLUTE  = 1 << 1,
};

enum FileEventType {
    FE_UNKNOWN = 0,
    FE_MODIFY,
    FE_CREATE,
    FE_DELETE,
};

struct FileEvent {
    FileEventType type;
    String path;
};

FileInfo read_file(String path, Allocator mem, i32 retry_count = 0);

void list_files(DynamicArray<String> *dst, String dir, String ext, Allocator mem, u32 flags = 0);

inline void list_files(DynamicArray<String> *dst, String dir, Allocator mem, u32 flags = 0)
{
    list_files(dst, dir, "", mem, flags);
}

inline DynamicArray<String> list_files(String dir, Allocator mem, u32 flags = 0)
{
    DynamicArray<String> files{ .alloc = mem };
    list_files(&files, dir, "", mem, flags);
    return files;
}

void list_folders(DynamicArray<String> *dst, String dir, Allocator mem, u32 flags = 0);
inline DynamicArray<String> list_folders(String dir, Allocator mem, u32 flags = 0)
{
	DynamicArray<String> dirs{ .alloc = mem };
	list_folders(&dirs, dir, mem, flags);
	return dirs;
}

void create_filewatch(String folder, DynamicArray<FileEvent> *events, Mutex *events_mutex);

String absolute_path(String relative, Allocator mem);

FileHandle open_file(String path, FileOpenMode mode);
void write_file(FileHandle handle, char *data, i32 bytes);
void close_file(FileHandle handle);

void write_file(String path, void *data, i32 bytes);
void write_file(String path, StringBuilder *sb);

bool is_directory(String path);
bool file_exists_sz(const char *path);
bool file_exists(String path);

void remove_file(String path);

String get_exe_folder(Allocator mem);
String get_working_dir(Allocator mem);
void set_working_dir(String path);

u64 file_modified_timestamp(String path);

String select_folder_dialog(Allocator mem);

#endif //FILE_H
