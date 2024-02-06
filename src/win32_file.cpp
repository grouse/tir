#include "file.h"
#include "win32_core.h"
#include "win32_shlwapi.h"

#include "string.h"
#include "gen/string.h"

#include <stdio.h>

const char* win32_string_from_file_attribute(DWORD dwFileAttribute)
{
    switch (dwFileAttribute) {
    case FILE_ATTRIBUTE_NORMAL: return "FILE_ATTRIBUTE_NORMAL";
    case FILE_ATTRIBUTE_DIRECTORY: return "FILE_ATTRIBUTE_DIRECTORY";
    case FILE_ATTRIBUTE_ARCHIVE: return "FILE_ATTRIBUTE_ARCHIVE";
    default:
        LOG_ERROR("unknown file attribute: 0x%x", dwFileAttribute);
        return "unknown";
    }
}


String absolute_path(String relative, Allocator mem)
{
    SArena scratch = tl_scratch_arena(mem);
    const char *sz_relative = sz_string(relative, scratch);

    // NOTE(jesper): size is total size required including null terminator
    DWORD size = GetFullPathNameA(sz_relative, 0, NULL, NULL);
    char *pstr = ALLOC_ARR(mem, char, size);

    // NOTE(jesper): length is size excluding null terminator
    DWORD length = GetFullPathNameA(sz_relative, size, pstr, NULL);
    return String{ pstr, (i32)length };
}

FileInfo read_file(String path, Allocator mem, i32 retry_count)
{
    SArena scratch = tl_scratch_arena(mem);
    char *sz_path = sz_string(path, scratch);

    FileInfo fi{};

    // TODO(jesper): when is this actually needed? read up on win32 file path docs
    char *ptr = sz_path;
    char *ptr_end = ptr + path.length;
    while (ptr < ptr_end) {
        if (*ptr == '/') {
            *ptr = '\\';
        }
        ptr++;
    }

    HANDLE file = CreateFileA(
        sz_path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION) {
        for (i32 i = 0; file == INVALID_HANDLE_VALUE && i < retry_count; i++) {
            LOG_INFO("failed opening file '%s' due to sharing violation, sleeping and retrying", sz_path);

            Sleep(5);
            file = CreateFileA(
                sz_path,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        }
    }

    if (file == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND) {
        LOG_INFO("file not found: %s", sz_path);
        return {};
    }

    if (file == INVALID_HANDLE_VALUE) {
        LOG_ERROR("failed to open file '%s': (%d) %s", sz_path, WIN32_ERR_STR);
        return {};
    }
    defer { CloseHandle(file); };

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        LOG_ERROR("failed getting file size for file '%s': (%d) %s", sz_path, WIN32_ERR_STR);
        return {};
    }

    PANIC_IF(
        file_size.QuadPart >= 0x7FFFFFFF,
        "error opening file '%s': file size (%lld bytes) exceeds maximum supported %d", sz_path, file_size.QuadPart, 0x7FFFFFFF);

    fi.size = file_size.QuadPart;
    fi.data = (u8*)ALLOC(mem, file_size.QuadPart);

    DWORD bytes_read;
    if (!ReadFile(file, fi.data, (DWORD)file_size.QuadPart, &bytes_read, 0)) {
        LOG_ERROR("failed reading file '%s': (%d) %s", sz_path, WIN32_ERR_STR);
        FREE(mem, fi.data);
        return {};
    }

    return fi;
}

HANDLE win32_open_file(wchar_t *wsz_path, u32 creation_mode, u32 access_mode)
{
    HANDLE file = CreateFileW(
        wsz_path,
        access_mode,
        0,
        nullptr,
        creation_mode,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE &&
        (creation_mode == CREATE_ALWAYS ||
         creation_mode == CREATE_NEW))
    {
        DWORD code = GetLastError();
        if (code == 3) {
            wchar_t *ptr = wsz_path;
            wchar_t *end = ptr + wcslen(ptr);

            while (ptr < end) {
                if (*ptr == '\\' || *ptr == '/') {
                    char c = *ptr;
                    *ptr = '\0';
                    defer{ *ptr = c; };

                    if (CreateDirectoryW(wsz_path, NULL) == 0) {
                        DWORD create_dir_error = GetLastError();
                        if (create_dir_error != ERROR_ALREADY_EXISTS) {
                            LOG_ERROR("failed creating folder: %ls, code: %d, msg: '%s'",
                                      wsz_path,
                                      create_dir_error,
                                      win32_system_error_message(create_dir_error));
                            return INVALID_HANDLE_VALUE;
                        }
                    }
                }
                ptr++;
            }
        } else {
            LOG_ERROR("failed creating file: '%ls', code: %d, msg: '%s'", wsz_path, code, win32_system_error_message(code));
            return INVALID_HANDLE_VALUE;
        }
    }

    return file;
}

HANDLE win32_open_file(char *sz_path, u32 creation_mode, u32 access_mode)
{
    HANDLE file = CreateFileA(
        sz_path,
        access_mode,
        0,
        nullptr,
        creation_mode,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE &&
        (creation_mode == CREATE_ALWAYS ||
         creation_mode == CREATE_NEW))
    {
        DWORD code = GetLastError();
        if (code == 3) {
            char *ptr = sz_path;
            char *end = ptr + strlen(ptr);

            while (ptr < end) {
                if (*ptr == '\\' || *ptr == '/') {
                    char c = *ptr;
                    *ptr = '\0';
                    defer{ *ptr = c; };

                    if (CreateDirectoryA(sz_path, NULL) == 0) {
                        DWORD create_dir_error = GetLastError();
                        if (create_dir_error != ERROR_ALREADY_EXISTS) {
                            LOG_ERROR("failed creating folder: %s, code: %d, msg: '%s'",
                                      sz_path,
                                      create_dir_error,
                                      win32_system_error_message(create_dir_error));
                            return INVALID_HANDLE_VALUE;
                        }
                    }
                }
                ptr++;
            }
        } else {
            LOG_ERROR("failed creating file: '%s', code: %d, msg: '%s'", sz_path, code, win32_system_error_message(code));
            return INVALID_HANDLE_VALUE;
        }
    }

    return file;
}

bool is_directory(String path)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);
    DWORD attribs = GetFileAttributesA(sz_path);
    return attribs == FILE_ATTRIBUTE_DIRECTORY;
}

void list_files(DynamicArray<String> *dst, String dir, String ext, Allocator mem, u32 flags)
{
    SArena scratch = tl_scratch_arena(mem);

    DynamicArray<char*> folders{ .alloc = scratch };
    array_add(&folders, sz_string(dir, scratch));

    char f[WIN32_MAX_PATH];
    for (i32 i = 0; i < folders.count; i++) {
        char *folder = folders[i];

        String fs = stringf(f, sizeof f, "%s/*", folder);
        f[fs.length] = '\0';

        WIN32_FIND_DATAA ffd{};
        HANDLE ff = FindFirstFileA(f, &ffd);

        do {
            if (ffd.cFileName[0] == '.') continue;

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (flags & FILE_LIST_RECURSIVE) {
                    char *child = join_path(folder, ffd.cFileName, scratch);
                    array_add(&folders, child);
                }
            } else if (ffd.dwFileAttributes & (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE)) {
                String filename{ ffd.cFileName, (i32)strlen(ffd.cFileName) };
                if (ext && !ends_with(filename, ext)) continue;

                String path;
                if (flags & FILE_LIST_ABSOLUTE) {
                    path = join_path(String{ folder, (i32)strlen(folder) }, filename, scratch);
                    path = absolute_path(path, mem);
                } else {
                    path = join_path(String{ folder, (i32)strlen(folder) }, filename, mem);
                }
                array_add(dst, path);
            } else {
                LOG_ERROR("unsupported file attribute for file '%s': %s",
                          ffd.cFileName, win32_string_from_file_attribute(ffd.dwFileAttributes));
            }
        } while (FindNextFileA(ff, &ffd));
    }
}

void list_folders(DynamicArray<String> *dst, String dir, Allocator mem, u32 flags)
{
    SArena scratch = tl_scratch_arena(mem);

    if (flags & FILE_LIST_ABSOLUTE) LOG_ERROR("unimplemented");

    DynamicArray<char*> folders{ .alloc = scratch };
    array_add(&folders, sz_string(dir, scratch));

    char f[WIN32_MAX_PATH];
    for (i32 i = 0; i < folders.count; i++) {
        char *folder = folders[i];

        String fs = stringf(f, sizeof f, "%s/*", folder);
        f[fs.length] = '\0';

        WIN32_FIND_DATAA ffd{};
        HANDLE ff = FindFirstFileA(f, &ffd);

        do {
            if (ffd.cFileName[0] == '.') continue;

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                char *subfolder = join_path(folder, ffd.cFileName, mem);
                array_add(dst, String{ subfolder, (i32)strlen(subfolder) });
                if (flags & FILE_LIST_RECURSIVE) array_add(&folders, subfolder);
            }
        } while (FindNextFileA(ff, &ffd));
    }
}

void write_file(String path, StringBuilder *sb)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);

    HANDLE file = win32_open_file(sz_path, CREATE_ALWAYS, GENERIC_WRITE);
    defer{ CloseHandle(file); };

    StringBuilder::Block *block = &sb->head;
    while (block && block->written > 0) {
        WriteFile(file, block->data, block->written, nullptr, nullptr);
        block = block->next;
    }
}

void write_file(String path, void *data, i32 size)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);

    HANDLE file = win32_open_file(sz_path, CREATE_ALWAYS, GENERIC_WRITE);
    defer{ CloseHandle(file); };

    WriteFile(file, data, size, NULL, NULL);
}

void create_filewatch(String folder, DynamicArray<FileEvent> *events, Mutex *events_mutex)
{
    String cfolders[] = { folder };
    Array<String> folders = { .data = &cfolders[0], .count = ARRAY_COUNT(cfolders) };

    struct FileWatchThreadData {
        String folder;
        DynamicArray<FileEvent> *events;
        Mutex *events_mutex;
    };

    auto thread_proc = [](void *user_data) -> i32
    {
        FileWatchThreadData *ftd = (FileWatchThreadData *)user_data;
        char *sz_dir = sz_string(ftd->folder, mem_dynamic);

        HANDLE h = CreateFileA(
            sz_dir,
            GENERIC_READ | FILE_LIST_DIRECTORY,
            FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
            NULL);

        if (h == INVALID_HANDLE_VALUE) {
            LOG_ERROR("unable to open file handle for directory '%s': (%d) %s", sz_dir, WIN32_ERR_STR);
            return 1;
        }

        DWORD buffer[sizeof(FILE_NOTIFY_INFORMATION)*2];
        while (true) {
            SArena scratch = tl_scratch_arena();

            DWORD num_bytes;
            BOOL result = ReadDirectoryChangesW(
                h,
                &buffer, sizeof buffer,
                true,
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                &num_bytes,
                NULL,
                NULL);

            if (!result) {
                LOG_ERROR("ReadDirectoryChangesW for dir '%s' failed: (%d) %s", sz_dir, WIN32_ERR_STR);
                CloseHandle(h);
                return 1;
            }

            FILE_NOTIFY_INFORMATION *fni = (FILE_NOTIFY_INFORMATION*)buffer;
            String sub_path = string_from_utf16((u16*)fni->FileName, fni->FileNameLength/2, scratch);
            String path = absolute_path(join_path(ftd->folder, sub_path, scratch), mem_dynamic);

            if (is_directory(path)) goto next_fni;

handle_fni:
            if (fni->Action == FILE_ACTION_MODIFIED || fni->Action == FILE_ACTION_ADDED) {
                GUARD_MUTEX(ftd->events_mutex) {
                    FileEvent event{};
                    for (i32 i = 0; i < ftd->events->count; i++) {
                        FileEvent fci = ftd->events->data[i];

                        if (fci.path == path) {
                            if (fci.type == FE_DELETE) array_remove_unsorted(ftd->events, i);
                            goto skip_add;
                        }
                    }

                    LOG_INFO("detected file %s: %.*s", fni->Action == FILE_ACTION_MODIFIED ? "modified" : "add", STRFMT(path));

                    event = {
                        .type = fni->Action == FILE_ACTION_ADDED ? FE_CREATE : FE_MODIFY,
                        .path = path,
                    };
                    array_add(ftd->events, event);
skip_add:;
                }
            } else if (fni->Action == FILE_ACTION_REMOVED) {
                LOG_INFO("detected file removal: %.*s", STRFMT(path));
                GUARD_MUTEX(ftd->events_mutex) {
                    for (i32 i = 0; i < ftd->events->count; i++) {
                        FileEvent fci = ftd->events->data[i];
                        if (fci.path == path) {
                            if (fci.type != FE_DELETE) array_remove_unsorted(ftd->events, i);
                            goto skip_add_remove;
                        }
                    }

                    array_add(ftd->events, { .type = FE_DELETE, .path = path });
skip_add_remove:;
                }

            }

next_fni:
            if (fni->NextEntryOffset > 0) {
                fni = (FILE_NOTIFY_INFORMATION*)(((u8*)fni) + fni->NextEntryOffset);
                goto handle_fni;
            }
        }

        CloseHandle(h);
        return 0;
    };

    for (String folder : folders) {
        FileWatchThreadData *ftd = ALLOC_T(mem_dynamic, FileWatchThreadData) {
            .folder = duplicate_string(folder, mem_dynamic),
            .events = events,
            .events_mutex = events_mutex,
        };

        create_thread(thread_proc, ftd);
    }
}

u64 file_modified_timestamp(String path)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);

    HANDLE file = win32_open_file(sz_path, OPEN_EXISTING, GENERIC_READ);

    if (file == INVALID_HANDLE_VALUE) {
        LOG_ERROR("unable to open file to get modified timestamp: '%.*s' - (%d) '%s'", STRFMT(path), WIN32_ERR_STR);
        return -1;
    }

    defer { CloseHandle(file); };

    FILETIME last_write_time;
    if (GetFileTime(file, nullptr, nullptr, &last_write_time)) {
        return last_write_time.dwLowDateTime | ((u64)last_write_time.dwHighDateTime << 32);
    }

    return -1;
}

void remove_file(String path)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);
    DeleteFileA(sz_path);
}

bool file_exists_sz(const char *path)
{
    return PathFileExistsA(path);
}

bool file_exists(String path)
{
    SArena scratch = tl_scratch_arena();
    const char *sz_path = sz_string(path, scratch);
    return file_exists_sz(sz_path);
}

FileHandle open_file(String path, FileOpenMode mode)
{
    SArena scratch = tl_scratch_arena();
    char *sz_path = sz_string(path, scratch);

    u32 creation_mode = 0;
    switch (mode) {
    case FILE_OPEN_CREATE:
        creation_mode = CREATE_NEW;
        break;
    case FILE_OPEN_TRUNCATE:
        creation_mode = CREATE_ALWAYS;
        break;
    }

    PANIC_IF(creation_mode == 0, "invalid creation mode");
    return win32_open_file(sz_path, creation_mode, GENERIC_WRITE|GENERIC_READ);
}

String file_path(FileHandle fh, Allocator mem)
{
    HANDLE handle = (HANDLE)fh;

    DWORD length = GetFinalPathNameByHandleA(handle, NULL, 0, FILE_NAME_NORMALIZED);
    if (length == 0) {
        LOG_ERROR("unable to get final path name by handle: (%d) %s", WIN32_ERR_STR);
        return {};
    }

    char *buffer = ALLOC_ARR(mem, char, length+1);
    length = GetFinalPathNameByHandleA(handle, buffer, length+1, FILE_NAME_NORMALIZED);

    String path{ buffer, (i32)length };
    if (starts_with(path, "\\\\?\\")) path = slice(path, 4);
    return path;
}

FileHandle create_temporary_file(const char *name, const char *suffix)
{
    SArena scratch = tl_scratch_arena();

    DWORD temp_len = GetTempPathW(0, NULL);
    wchar_t *temp_path = ALLOC_ARR(*scratch, wchar_t, temp_len+1);
    GetTempPathW(temp_len, temp_path);

    i32 prefix_len, suffix_len;
    wchar_t *wsz_prefix = wsz_string(string(name), &prefix_len, scratch);
    wchar_t *wsz_suffix = wsz_string(string(suffix), &suffix_len, scratch);

    wchar_t wsz_path[WIN32_MAX_PATH];
    memcpy(wsz_path, temp_path, temp_len*sizeof(wchar_t));
    memcpy(wsz_path+temp_len-1, wsz_prefix, prefix_len*sizeof(wchar_t));

    u16 uuid = (u16)wall_timestamp();

    HANDLE handle = INVALID_HANDLE_VALUE;
    while (handle == INVALID_HANDLE_VALUE) {
        wchar_t wsz_uuid[6];
        i32 uuid_len = swprintf(wsz_uuid, ARRAY_COUNT(wsz_uuid), L"%llu", uuid);

        wchar_t *ptr = wsz_path + temp_len-1 + prefix_len;
        memcpy(ptr, wsz_uuid, uuid_len*sizeof(wchar_t));
        ptr += uuid_len;

        memcpy(ptr, wsz_suffix, suffix_len*sizeof(wchar_t));
        ptr += suffix_len;

        *ptr = '\0';

        handle = win32_open_file(wsz_path, CREATE_NEW, GENERIC_WRITE|GENERIC_READ);
        uuid += 1;
    }

    return handle;
}

void write_file(FileHandle handle, const char *data, i32 bytes)
{
    WriteFile(handle, data, bytes, nullptr, nullptr);
}

void close_file(FileHandle handle)
{
    CloseHandle(handle);
}

String get_exe_folder(Allocator mem)
{
    wchar_t buffer[255];
    i32 size = ARRAY_COUNT(buffer);

    wchar_t *sw = buffer;
    i32 length = GetModuleFileNameW(NULL, sw, size);

    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        if (sw == buffer) sw = ALLOC_ARR(mem, wchar_t, size+10);
        else sw = REALLOC_ARR(mem, wchar_t, sw, size, size+10);

        size += 10;
        length = GetModuleFileNameW(NULL, sw, size);
    }

    for (wchar_t *p = sw+length; p >= sw; p--) {
        if (*p == '\\') {
            length = (i32)(p - sw);
            break;
        }
    }

    String s = string_from_utf16(sw, length, mem);
    return s;
}

String get_working_dir(Allocator mem)
{
    SArena scratch = tl_scratch_arena(mem);

    wchar_t buffer[255];
    i32 size = ARRAY_COUNT(buffer);

    wchar_t *sw = buffer;
    i32 length = GetCurrentDirectoryW(size, sw);

    if (length > size) {
        i32 req_size = length;
        sw = ALLOC_ARR(*scratch, wchar_t, req_size);
        length = GetCurrentDirectoryW(req_size, sw);
    }

    String s = string_from_utf16(sw, length, mem);
    return s;
}

void set_working_dir(String path)
{
    SArena scratch = tl_scratch_arena();
    wchar_t *wsz_path = wsz_string(path, scratch);

    if (!SetCurrentDirectoryW(wsz_path)) {
        LOG_ERROR("unable to change working dir to '%S': (%d) %s", wsz_path, WIN32_ERR_STR);
    }
}

#if 0
String select_folder_dialog(Allocator mem)
{
    SArena scratch = tl_scratch_memory(mem);
    extern HWND win32_root_window;

    wchar_t display_name_buffer[WIN32_MAX_PATH];
    BROWSEINFOW bi{
        .hwndOwner = win32_root_window,
        .pszDisplayName = display_name_buffer,
        .lpszTitle = L"Select folder",
        .ulFlags = BIF_USENEWUI|BIF_RETURNONLYFSDIRS,
    };

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl == NULL) return "";

    wchar_t buffer[WIN32_MAX_PATH];

    wchar_t *dst = buffer;
    i32 dst_count = ARRAY_COUNT(buffer);

    while (!SHGetPathFromIDListEx(pidl, dst, dst_count, 0)) {
        if (dst == buffer) dst = ALLOC_ARR(*scratch, wchar_t, dst_count+10);
        else dst = REALLOC_ARR(*scratch, wchar_t, dst, dst_count, dst_count+10);
        dst_count += 10;
    }

    return string_from_utf16(dst, wcslen(dst), mem);
}
#endif
