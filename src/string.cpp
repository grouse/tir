#include "string.h"

#include "core.h"

#include <stdarg.h>
#include <stdio.h>

bool operator!=(String lhs, String rhs)
{
    return lhs.length != rhs.length || memcmp(lhs.data, rhs.data, lhs.length) != 0;
}

bool operator==(String lhs, String rhs)
{
    return lhs.length == rhs.length && memcmp(lhs.data, rhs.data, lhs.length) == 0;
}

String create_string(const char *str, i32 length, Allocator mem)
{
    String s;
    s.data = (char*)ALLOC(mem, length);
    s.length = length;
    memcpy(s.data, str, length);
    return s;
}

String duplicate_string(String other, Allocator mem)
{
    String str;
    str.data = (char*)ALLOC(mem, other.length);
    memcpy(str.data, other.data, other.length);
    str.length = other.length;
    return str;
}

void string_copy(String *dst, String src, Allocator mem)
{
    dst->data = (char*)REALLOC(mem, dst->data, dst->length, src.length);
    memcpy(dst->data, src.data, src.length);
    dst->length = src.length;
}

String stringf(char *buffer, i32 size, const char *fmt, ...)
{
    String result{};

    va_list args;
    va_start(args, fmt);

    i32 length = vsnprintf(buffer, size-1, fmt, args);
    if (length <= size) {
        result.data = buffer;
        result.length = length;
    }

    va_end(args);
    return result;
}

String stringf(Allocator mem, const char *fmt, ...)
{
    String result{};

    va_list args;
    va_start(args, fmt);
    i32 length = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    va_start(args, fmt);
    result.data = (char*)ALLOC(mem, length+1);
    result.length = vsnprintf(result.data, length+1, fmt, args);
    va_end(args);

    return result;
}

String slice(String str, i32 start, i32 end)
{
    String r;
    r.data = str.data+start;
    r.length = end-start;
    return r;
}

String slice(String str, i32 start)
{
    String r;
    r.data = str.data+start;
    r.length = str.length-start;
    return r;
}

i32 i32_from_string(String s)
{
    if (i32 value; i32_from_string(s, &value)) return value;
    return -1;
}

bool i32_from_string(String s, i32 *dst)
{
    i32 sign = 1;
    i32 result = 0;

    i32 i = 0;
    if (s[0] == '-') {
        sign = -1;
        i = 1;
    }

    for (; i < s.length; i++) {
        result = result * 10 + (s[i] - '0');
    }

    *dst = sign*result;
    return true;
}

bool f32_from_string(String s, f32 *dst)
{
    SArena scratch = tl_scratch_arena();
    char *sz_s = sz_string(s, scratch);
    int r = sscanf(sz_s, "%f", dst);
    return r == 1;
}

bool starts_with(String lhs, String rhs)
{
    return lhs.length >= rhs.length && memcmp(lhs.data, rhs.data, rhs.length) == 0;
}

bool starts_with(const char *lhs, const char *rhs)
{
	return strncmp(lhs, rhs, strlen(lhs)) == 0;
}

bool ends_with(String lhs, String rhs)
{
    return lhs.length >= rhs.length && memcmp(lhs.data+lhs.length-rhs.length, rhs.data, rhs.length) == 0;
}

char* sz_string(String str, Allocator mem)
{
    if (str.length == 0) return nullptr;

    char *sz_str = ALLOC_ARR(mem, char, str.length+1);
    memcpy(sz_str, str.data, str.length);
    sz_str[str.length] = '\0';
    return sz_str;
}

i32 utf8_truncate(String str, i32 limit)
{
    if (str.length < limit) return str.length;

    i32 truncated = 0;
    for (i32 i = 0; i < str.length; i++) {
        char c = str[i++];

        if (c & 0x80) {
            if (~c & (1 << 5))      i += 1;
            else if (~c & (1 << 4)) i += 2;
            else if (~c & (1 << 3)) i += 3;
        }

        if (i > limit) return truncated;;
        truncated = i;
    }

    return truncated;
}

i32 utf8_from_utf16(u8 *dst, i32 capacity, const u16 *src, i32 length)
{
    i32 written = 0;
    for (const u16 *ptr = src; ptr < src+length; ptr++) {
        u16 utf16 = *ptr;

        u16 utf16_hi_surrogate_start = 0xD800;
        u16 utf16_lo_surrogate_start = 0xDC00;
        u16 utf16_surrogate_end = 0xDFFF;

        u16 high_surrogate = 0;
        if (utf16 >= utf16_hi_surrogate_start &&
            utf16 < utf16_lo_surrogate_start)
        {
            high_surrogate = utf16;
            utf16 = *ptr++;
        }

        u32 utf32 = utf16;
        if (utf16 >= utf16_lo_surrogate_start &&
            utf16 <= utf16_surrogate_end)
        {
            utf32  = (high_surrogate - utf16_hi_surrogate_start) << 10;
            utf32 |= utf16;
            utf32 += 0x1000000;
        }

        if (utf32 <= 0x7F) {
            if (written+1 > capacity) return written;
            dst[written++] = (u8)utf32;
        } else if (utf32 <= 0x7FF) {
            if (written+2 > capacity) return written;
            dst[written++] = (u8)(0b11000000 | (0b00011111 & (u8)(utf32 >> 6)));
            dst[written++] = (u8)(0b10000000 | (0b00111111 & (u8)(utf32)));
        } else if (utf32 <= 0xFFFF) {
            if (written+3 > capacity) return written;
            dst[written++] = (u8)(0b11100000 | (0b00001111 & (u8)(utf32 >> 12)));
            dst[written++] = (u8)(0b10000000 | (0b00111111 & (u8)(utf32 >> 6)));
            dst[written++] = (u8)(0b10000000 | (0b00111111 & (u8)(utf32)));
        } else {
            if (written+4 > capacity) return written;
            dst[written++] = (u8)(0b11110000 | (0b00000111 & (u8)(utf32 >> 18)));
            dst[written++] = (u8)(0b10000000 | (0b00111111 & (u8)(utf32 >> 12)));
            dst[written++] = (u8)(0b10000000 | (0b00111111 & (u8)(utf32 >> 6)));
            dst[written++] = (u8)(0b10000000 | (0b00111111 & (u8)(utf32)));
        }
    }

    return written;
}

String string_from_utf16(const u16 *in_str, i32 length, Allocator mem)
{
    i32 capacity = length;

    String str = {};
    str.data = (char*)ALLOC(mem, capacity);

    for (const u16 *ptr = in_str; ptr < in_str+length; ptr++) {
        u16 utf16 = *ptr;

        u16 utf16_hi_surrogate_start = 0xD800;
        u16 utf16_lo_surrogate_start = 0xDC00;
        u16 utf16_surrogate_end = 0xDFFF;

        u16 high_surrogate = 0;
        if (utf16 >= utf16_hi_surrogate_start &&
            utf16 < utf16_lo_surrogate_start)
        {
            high_surrogate = utf16;
            utf16 = *ptr++;
        }

        u32 utf32 = utf16;
        if (utf16 >= utf16_lo_surrogate_start &&
            utf16 <= utf16_surrogate_end)
        {
            utf32  = (high_surrogate - utf16_hi_surrogate_start) << 10;
            utf32 |= utf16;
            utf32 += 0x1000000;
        }

        if (utf32 <= 0x7F) {
            if (str.length + 1 > capacity) {
                i32 old_capacity = capacity;
                capacity = str.length + 1;
                str.data = (char*)REALLOC(mem, str.data, old_capacity, capacity);
            }

            str.data[str.length++] = (char)utf32;
        } else if (utf32 <= 0x7FF) {
            if (str.length + 2 > capacity) {
                i32 old_capacity = capacity;
                capacity = str.length + 2;
                str.data = (char*)REALLOC(mem, str.data, old_capacity, capacity);

            }

            str.data[str.length++] = (char)(0b11000000 | (0b00011111 & (u8)(utf32 >> 6)));
            str.data[str.length++] = (char)(0b10000000 | (0b00111111 & (u8)(utf32)));
        } else if (utf32 <= 0xFFFF) {
            if (str.length + 3 > capacity) {
                i32 old_capacity = capacity;
                capacity = str.length + 3;
                str.data = (char*)REALLOC(mem, str.data, old_capacity, capacity);
            }

            str.data[str.length++] = (char)(0b11100000 | (0b00001111 & (u8)(utf32 >> 12)));
            str.data[str.length++] = (char)(0b10000000 | (0b00111111 & (u8)(utf32 >> 6)));
            str.data[str.length++] = (char)(0b10000000 | (0b00111111 & (u8)(utf32)));
        } else {
            if (str.length + 4 > capacity) {
                i32 old_capacity = capacity;
                capacity = str.length + 4;
                str.data = (char*)REALLOC(mem, str.data, old_capacity, capacity);
            }

            str.data[str.length++] = (char)(0b11110000 | (0b00000111 & (u8)(utf32 >> 18)));
            str.data[str.length++] = (char)(0b10000000 | (0b00111111 & (u8)(utf32 >> 12)));
            str.data[str.length++] = (char)(0b10000000 | (0b00111111 & (u8)(utf32 >> 6)));
            str.data[str.length++] = (char)(0b10000000 | (0b00111111 & (u8)(utf32)));
        }
    }

    return str;
}

i32 utf8_length(const u16 *str, i32 utf16_len, i32 limit)
{
    i32 length = 0;
    for (i32 i = 0; i < utf16_len; i++) {
        u16 utf16 = *str;

        u16 utf16_hi_surrogate_start = 0xD800;
        u16 utf16_lo_surrogate_start = 0xDC00;
        u16 utf16_surrogate_end = 0xDFFF;

        u16 high_surrogate = 0;
        if (utf16 >= utf16_hi_surrogate_start &&
            utf16 < utf16_lo_surrogate_start)
        {
            high_surrogate = utf16;
            utf16 = *str++;
        }

        u32 utf32 = utf16;
        if (utf16 >= utf16_lo_surrogate_start &&
            utf16 <= utf16_surrogate_end)
        {
            utf32  = (high_surrogate - utf16_hi_surrogate_start) << 10;
            utf32 |= utf16;
            utf32 += 0x1000000;
        }

        if (utf32 <= 0x7F) {
            if (length+1 > limit) return length;
            length += 1;
        } else if (utf32 <= 0x7FF) {
            if (length+2 > limit) return length;
            length += 2;
        } else if (utf32 <= 0xFFFF) {
            if (length+3 > limit) return length;
            length += 3;
        } else {
            if (length+4 > limit) return length;
            length += 4;
        }
    }

    return length;
}

i32 utf8_length(const u16 *str, i32 utf16_len)
{
    i32 length = 0;
    for (i32 i = 0; i < utf16_len; i++) {
        u16 utf16 = *str;

        u16 utf16_hi_surrogate_start = 0xD800;
        u16 utf16_lo_surrogate_start = 0xDC00;
        u16 utf16_surrogate_end = 0xDFFF;

        u16 high_surrogate = 0;
        if (utf16 >= utf16_hi_surrogate_start &&
            utf16 < utf16_lo_surrogate_start)
        {
            high_surrogate = utf16;
            utf16 = *str++;
        }

        u32 utf32 = utf16;
        if (utf16 >= utf16_lo_surrogate_start &&
            utf16 <= utf16_surrogate_end)
        {
            utf32  = (high_surrogate - utf16_hi_surrogate_start) << 10;
            utf32 |= utf16;
            utf32 += 0x1000000;
        }

        if (utf32 <= 0x7F) {
            length += 1;
        } else if (utf32 <= 0x7FF) {
            length += 2;
        } else if (utf32 <= 0xFFFF) {
            length += 3;
        } else {
            length += 4;
        }
    }

    return length;
}

i32 utf16_length(String str)
{
    i32 length = 0;
    for (i32 i = 0; i < str.length; i++) {
        u32 c = (u32)str.data[i];

        if (c & 0x80) {
            if (~c & (1 << 5)) {
                if (i+1 >= str.length) goto end;

                c = (str.data[i] & 0b00011111) << 6 |
                    (str.data[i+1] & 0b00111111);
                i += 1;
            } else if (~c & (1 << 4)) {
                if (i+2 >= str.length) goto end;

                c = (str.data[i] & 0b00001111) << 12 |
                    (str.data[i+1] & 0b00111111) << 6 |
                    (str.data[i+2] & 0b00111111);
                i += 2;
            } else if (~c & (1 << 3)) {
                if (i+3 >= str.length) goto end;

                c = (str.data[i] & 0b00000111) << 18 |
                    (str.data[i+1] & 0b00111111) << 12 |
                    (str.data[i+2] & 0b00111111) << 6 |
                    (str.data[i+3] & 0b00111111);
                i += 3;
            }
        }

        if ((c >= 0x0000 && c <= 0xD7FF) ||
            (c >= 0xE000 && c <= 0xFFFF))
        {
            length += 1;
        } else {
            length += 2;
        }
    }
end:
    return length;
}

void utf16_from_string(u16 *dst, i32 capacity, String src)
{
    i32 length = 0;
    for (i32 i = 0; i < src.length; i++) {
        u32 c = (u32)src.data[i];

        if (c & 0x80) {
            if (~c & (1 << 5)) {
                if (i+1 >= src.length) goto end;

                c = (src.data[i] & 0b00011111) << 6 |
                    (src.data[i+1] & 0b00111111);
                i += 1;
            } else if (~c & (1 << 4)) {
                if (i+2 >= src.length) goto end;

                c = (src.data[i] & 0b00001111) << 12 |
                    (src.data[i+1] & 0b00111111) << 6 |
                    (src.data[i+2] & 0b00111111);
                i += 2;
            } else if (~c & (1 << 3)) {
                if (i+3 >= src.length) goto end;

                c = (src.data[i] & 0b00000111) << 18 |
                    (src.data[i+1] & 0b00111111) << 12 |
                    (src.data[i+2] & 0b00111111) << 6 |
                    (src.data[i+3] & 0b00111111);
                i += 3;
            }
        }

        if ((c >= 0x0000 && c <= 0xD7FF) ||
            (c >= 0xE000 && c <= 0xFFFF))
        {
            if (length+1 > capacity) goto end;
            dst[length++] = (u16)c;
        } else {
            if (length+2 > capacity) goto end;
            u32 u = c - 0x10000;
            dst[length++] = 0b1101100000000000 | ((u >> 10) & 0b0000001111111111);
            dst[length++] = 0b1101110000000000 | (u         & 0b0000001111111111);
        }
    }

end:
    return;
}

u16* utf16_from_string(String str, i32 *utf16_length, Allocator mem)
{
    i32 length = 0;
    i32 capacity = str.length;
    u16 *utf16 = (u16*)ALLOC(mem, str.length * sizeof *utf16);

    for (i32 i = 0; i < str.length; i++) {
        u32 c = (u32)str.data[i];

        if (c & 0x80) {
            if (~c & (1 << 5)) {
                if (i+1 >= str.length) goto end;

                c = (str.data[i] & 0b00011111) << 6 |
                    (str.data[i+1] & 0b00111111);
                i += 1;
            } else if (~c & (1 << 4)) {
                if (i+2 >= str.length) goto end;

                c = (str.data[i] & 0b00001111) << 12 |
                    (str.data[i+1] & 0b00111111) << 6 |
                    (str.data[i+2] & 0b00111111);
                i += 2;
            } else if (~c & (1 << 3)) {
                if (i+3 >= str.length) goto end;

                c = (str.data[i] & 0b00000111) << 18 |
                    (str.data[i+1] & 0b00111111) << 12 |
                    (str.data[i+2] & 0b00111111) << 6 |
                    (str.data[i+3] & 0b00111111);
                i += 3;
            }
        }

        if ((c >= 0x0000 && c <= 0xD7FF) ||
            (c >= 0xE000 && c <= 0xFFFF))
        {
            if (length+1 > capacity) {
                i32 old_capacity = capacity;
                capacity = length+1;
                utf16 = (u16*)REALLOC(mem, utf16, old_capacity * sizeof *utf16, capacity * sizeof *utf16);
            }

            utf16[length++] = (u16) c;
        } else {
            if (length+2 > capacity) {
                i32 old_capacity = capacity;
                capacity = length+2;
                utf16 = (u16*)REALLOC(mem, utf16, old_capacity * sizeof *utf16, capacity * sizeof *utf16);
            }
            u32 u = c - 0x10000;
            utf16[length++] = 0b1101100000000000 | ((u >> 10) & 0b0000001111111111);
            utf16[length++] = 0b1101110000000000 | (u         & 0b0000001111111111);
        }
    }

end:
    *utf16_length = length;
    return utf16;
}

i32 byte_index_from_codepoint_index(String str, i32 codepoint)
{
    i32 i, ci;
    for (i = 0, ci = 0; i < str.length; i++, ci++) {
        if (ci == codepoint) return i;

        char c = str[i];
        if (c & 0x80) {
            if (~c & (1 << 5))      i += 1;
            else if (~c & (1 << 4)) i += 2;
            else if (~c & (1 << 3)) i += 3;
        }
    }
    return i;
}

i32 codepoint_index_from_byte_index(String str, i32 byte)
{
    i32 ci = 0;
    for (i32 i = 0; i < str.length; i++) {
        if (i == byte) return ci;
        ci++;

        char c = str[i];
        if (c & 0x80) {
            if (~c & (1 << 5))      i += 1;
            else if (~c & (1 << 4)) i += 2;
            else if (~c & (1 << 3)) i += 3;
        }
    }
    return ci;
}

i64 utf8_decr(char *str, i64 i)
{
    i--;
    while (i > 0 && (str[i] & 0b11000000) == 0b10000000) i--;
    return i;
}

i64 utf8_incr(char *str, i64 length, i64 i)
{
    if (i < length) {
        char c = str[i++];
        if (c & 0x80) {
            if (~c & (1 << 5))      i += 1;
            else if (~c & (1 << 4)) i += 2;
            else if (~c & (1 << 3)) i += 3;
        }
    }
    return i;
}

i32 utf8_decr(String str, i32 i)
{
    return (i32)utf8_decr(str.data, i);
}

i32 utf8_incr(String str, i32 i)
{
    return (i32)utf8_incr(str.data, str.length, i);
}


bool path_equals(String lhs, String rhs) EXPORT
{
    if (lhs.length != rhs.length) return false;

    for (i32 i = 0; i < lhs.length; i++) {
        if (lhs.data[i] != rhs.data[i] &&
            (lhs.data[i] != '/' || rhs.data[i] != '\\') &&
            (lhs.data[i] != '\\' || rhs.data[i] != '/'))
        {
            return false;
        }
    }

    return true;
}

String extension_of(String path) EXPORT
{
    for (i32 i = path.length-1; i >= 0; i--) {
        if (path.data[i] == '.') {
            return String{ path.data+i, path.length-i };
        }
    }

    return {};
}

String filename_of(String path) EXPORT
{
    for (i32 i = path.length-1; i >= 0; i--) {
        if (path.data[i] == '/' || path.data[i] == '\\') {
            return String{ path.data+i+1, path.length-(i+1) };
        }
    }

    return path;
}

String filename_of_sz(const char *path) EXPORT
{
    const char *p = path;

    for (; *p; p++) {
        if (*p == '/' || *p == '\\') {
            path = p+1;
        }
    }

    return { path, i32(p-path) };
}

String directory_of(String path) EXPORT
{
    for (i32 i = path.length-1; i >= 0; i--) {
        if (path.data[i] == '/' || path.data[i] == '\\') {
            return slice(path, 0, i);
        }
    }

    return path;
}

String path_relative_to(String path, String root)
{
    ASSERT(path.length > root.length);
    String proot = slice(path, 0, root.length);
    ASSERT(proot == root);
    String short_path = slice(path, root.length, path.length);
    while (short_path.length > 0 &&
           (short_path.data[0] == '/' || short_path.data[0] == '\\'))
    {
        short_path.data++;
        short_path.length--;
    }
    return short_path;
}

String join_path(String root, String filename, Allocator mem) EXPORT
{
    bool add_slash = false;
    i32 required_length = root.length + filename.length;

    if (root[root.length-1] != '/' && filename[0] != '/' &&
        root[root.length-1] != '\\' && filename[0] != '/')
    {
        required_length += 1;
        add_slash = true;
    }

    String path;
    path.data = (char*)ALLOC(mem, required_length);
    memcpy(path.data, root.data, root.length);
    path.length = root.length;

    if (add_slash) path[path.length++] = '/';

    memcpy(path.data+path.length, filename.data, filename.length);
    path.length += filename.length;
    return path;
}

char* join_path(const char *sz_root, const char *sz_filename, Allocator mem) EXPORT
{
    i32 root_length = strlen(sz_root);
    i32 filename_length = strlen(sz_filename);

    bool add_slash = false;
    i32 required_length = root_length + filename_length;

    if (sz_root[root_length-1] != '/' && sz_filename[0] != '/' &&
        sz_root[root_length-1] != '\\' && sz_filename[0] != '/')
    {
        required_length += 1;
        add_slash = true;
    }

    char *sz_path = (char*)ALLOC(mem, required_length+1);
    memcpy(sz_path, sz_root, root_length);
    i32 path_length = root_length;

    if (add_slash) sz_path[path_length++] = '/';

    memcpy(sz_path+path_length, sz_filename, filename_length);
    path_length += filename_length;
    sz_path[path_length] = '\0';
    return sz_path;
}

i32 utf8_from_utf32(u8 utf8[4], i32 utf32)
{
    if (utf32 <= 0x007F) {
        utf8[0] = (u8)utf32;
        return 1;
    } else if (utf32 <= 0x7FF) {
        utf8[0] = 0b11000000 | ((utf32 >> 6) & 0b00011111);
        utf8[1] = 0b10000000 | (utf32 & 0b00111111);
        return 2;
    } else if (utf32 <= 0xFFFF) {
        utf8[0] = 0b11100000 | ((utf32 >> 12) & 0b00001111);
        utf8[1] = 0b10000000 | ((utf32 >> 6)  & 0b00111111);
        utf8[2] = 0b10000000 | (utf32 & 0b00111111);
        return 3;
    } else if (utf32 <= 0x10FFFF) {
        utf8[0] = 0b11110000 | ((utf32 >> 18) & 0b00001111);
        utf8[1] = 0b10000000 | ((utf32 >> 12) & 0b00111111);
        utf8[2] = 0b10000000 | ((utf32 >> 6)  & 0b00111111);
        utf8[3] = 0b10000000 | (utf32 & 0b00111111);
        return 4;
    }

    return -1;
}

i64 utf32_it_next(char *str, i64 length, i64 *offset)
{
    i32 c = str[*offset];
    i64 end = (i64)(str + length);

    if (c & 0x80) {
        if (~c & (1 << 5)) {
            if ((*offset)+1 >= end) return 0;

            c = (str[*offset] & 0b00011111) << 6 |
                (str[(*offset)+1] & 0b00111111);
            (*offset) += 1;
        } else if (~c & (1 << 4)) {
            if ((*offset)+2 >= end) return 0;

            c = (str[*offset] & 0b00001111) << 12 |
                (str[(*offset)+1] & 0b00111111) << 6 |
                (str[(*offset)+2] & 0b00111111);

            (*offset) += 2;
        } else if (~c & (1 << 3)) {
            if ((*offset)+3 >= end) return 0;

            c = (str[*offset] & 0b00000111) << 18 |
                (str[(*offset)+1] & 0b00111111) << 12 |
                (str[(*offset)+2] & 0b00111111) << 6 |
                (str[(*offset)+3] & 0b00111111);

            (*offset) += 3;
        }
    }

    (*offset) += 1;
    return c;
}

i32 utf32_it_next(String str, i32 *offset)
{
    i32 c = str[*offset];
    i64 end = (i64)(str.data + str.length);

    if (c & 0x80) {
        if (~c & (1 << 5)) {
            if ((*offset)+1 >= end) return 0;

            c = (str.data[*offset] & 0b00011111) << 6 |
                (str.data[(*offset)+1] & 0b00111111);
            (*offset) += 1;
        } else if (~c & (1 << 4)) {
            if ((*offset)+2 >= end) return 0;

            c = (str.data[*offset] & 0b00001111) << 12 |
                (str.data[(*offset)+1] & 0b00111111) << 6 |
                (str.data[(*offset)+2] & 0b00111111);

            (*offset) += 2;
        } else if (~c & (1 << 3)) {
            if ((*offset)+3 >= end) return 0;

            c = (str.data[*offset] & 0b00000111) << 18 |
                (str.data[(*offset)+1] & 0b00111111) << 12 |
                (str.data[(*offset)+2] & 0b00111111) << 6 |
                (str.data[(*offset)+3] & 0b00111111);

            (*offset) += 3;
        }
    }

    (*offset) += 1;
    return c;
}

i32 utf32_it_next(char **utf8, char *end)
{
    i32 c = **utf8;

    char *cur = *utf8;
    if (c & 0x80) {
        if (~c & (1 << 5)) {
            if ((*utf8)+1 >= end) return 0;

            c = (cur[0] & 0b00011111) << 6 |
                (cur[1] & 0b00111111);
            (*utf8) += 1;
        } else if (~c & (1 << 4)) {
            if ((*utf8)+2 >= end) return 0;

            c = (cur[0] & 0b00001111) << 12 |
                (cur[1] & 0b00111111) << 6 |
                (cur[2] & 0b00111111);

            (*utf8) += 2;
        } else if (~c & (1 << 3)) {
            if ((*utf8)+3 >= end) return 0;

            c = (cur[0] & 0b00000111) << 18 |
                (cur[1] & 0b00111111) << 12 |
                (cur[2] & 0b00111111) << 6 |
                (cur[3] & 0b00111111);

            (*utf8) += 3;
        }
    }

    (*utf8) += 1;
    return c;
}

void reset_string_builder(StringBuilder *sb)
{
    for (auto it = sb->current; it; it = it->next) {
        it->written = 0;
    }

    sb->current = &sb->head;
}

String create_string(StringBuilder *sb, Allocator mem)
{
    i32 length = 0;
    for (auto it = sb->current; it; it = it->next) {
        if (it->written == 0) break;
        length += it->written;
    }

    String str;
    str.data = (char*)ALLOC(mem, length);
    str.length = length;

    char *ptr = str.data;
    for (auto it = sb->current; it; it = it->next) {
        if (it->written == 0) break;
        memcpy(ptr, it->data, it->written);
    }

    return str;
}

void append_string(StringBuilder *sb, String str)
{
    i32 available = MIN((i32)sizeof sb->current->data - sb->current->written, str.length);
    i32 rest = str.length - available;

    memcpy(sb->current->data+sb->current->written, str.data, available);
    sb->current->written += available;

    i32 written = available;
    while(rest > 0) {
        if (sb->current->next == nullptr) {
            StringBuilder::Block *block = (StringBuilder::Block*)ALLOC(sb->alloc, sizeof(StringBuilder::Block));
            memset(block, 0, sizeof *block);

            sb->current->next = block;
        }

        sb->current = sb->current->next;
        sb->current->written = 0;

        i32 to_write = MIN((i32)sizeof sb->current->data, rest);
        memcpy(sb->current->data, str.data+written, to_write);
        sb->current->written += to_write;

        rest -= to_write;
        written += to_write;
    }
}

void append_stringf(StringBuilder *sb, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    i32 available = sizeof sb->current->data - sb->current->written;
    i32 length = vsnprintf(sb->current->data + sb->current->written, available-1, fmt, args);
    va_end(args);

    if (length > available-1) {
        SArena scratch = tl_scratch_arena(sb->alloc);
        char *buffer = (char*)ALLOC(*scratch, length+1);

        va_start(args, fmt);
        vsnprintf(buffer, length+1, fmt, args);
        va_end(args);

        append_string(sb, String{ buffer, length });
    } else {
        sb->current->written += length;
    }
}

bool is_whitespace(i32 c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_number(i32 c)
{
    return c >= '0' && c <= '9';
}

bool is_newline(i32 c)
{
    return c == '\n' || c == '\r';
}

String to_lower(String s, Allocator mem)
{
    String l = duplicate_string(s, mem);
    for (i32 i = 0; i < l.length; i++) l[i] = to_lower(l[i]);
    return l;
}

i32 last_of(String str, char c)
{
	i32 p = -1;
	for (i32 i = 0; i < str.length; i++) {
		if (str[i] == c) p = i;
	}
	return p;
}

void to_lower(String *s)
{
    for (i32 i = 0; i < s->length; i++) s->data[i] = to_lower(s->data[i]);
}

char to_lower(char c)
{
    return tolower(c);
}

char* last_of(char *str, char c)
{
	char *p = nullptr;
	while (*str) {
		if (*str == c) p = str;
		str++;
	}

	return p;
}

bool parse_cmd_argument(String *args, i32 count, String name, i32 values[2])
{
    for (i32 i = 0; i < count; i++) {
        if (args[i] == name) {
            if (i+2 >= count) return false;
            if (!i32_from_string(args[1], &values[0])) return false;
            if (!i32_from_string(args[2], &values[1])) return false;
            return true;
        }
    }

    return false;
}


#if defined(_WIN32)
i32 utf8_from_utf16(u8 *dst, i32 capacity, const wchar_t *src, i32 length)
{
    static_assert(sizeof(wchar_t) == sizeof(u16));
    return utf8_from_utf16(dst, capacity, (const u16*)src, length);
}

String string_from_utf16(const wchar_t *in_str, i32 length, Allocator mem)
{
    static_assert(sizeof(wchar_t) == sizeof(u16));
    return string_from_utf16((const u16*)in_str, length, mem);
}

i32 utf8_length(const wchar_t *str, i32 utf16_len)
{
    static_assert(sizeof(wchar_t) == sizeof(u16));
    return utf8_length((const u16*)str, utf16_len);
}

wchar_t* wsz_string(String str, Allocator mem)
{
    i32 wl = utf16_length(str);
    if (wl == 0) return nullptr;

    wchar_t *wsz_str = ALLOC_ARR(mem, wchar_t, wl+1);
    utf16_from_string((u16*)wsz_str, wl, str);
    wsz_str[wl] = L'\0';
    return wsz_str;
}
#endif
