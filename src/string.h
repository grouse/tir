#ifndef STRING_H
#define STRING_H

#include "platform.h"
#include "memory.h"

#define STRFMT(str) (str).length, (str).data

extern "C" size_t strlen(const char * str) NOTHROW;
extern "C" size_t wcslen(const wchar_t* wcs) NOTHROW;
extern "C" int strcmp(const char * str1, const char * str2) NOTHROW;
extern "C" int strncmp(const char * str1, const char * str2, size_t num) NOTHROW;
extern "C" int tolower(int ch);

struct String {
    char *data;
    i32 length;

    String() = default;
    String(const char *data, i32 length)
    {
        this->data = (char*)data;
        this->length = length;
    }

    template<i32 N>
    String(const char(&str)[N])
    {
        length = N-1;
        data = (char*)str;
    }

    char& operator[](i32 i)
    {
        // NOTE(jesper): disabled because C/C++ is garbage and assert is defined
        // in core.h but core.h needs to include string.h for its procedures. Need
        // to make them not rely on string or some nonsense
        //ASSERT(i < length && i >= 0);
        return data[i];
    }

    operator bool() const { return length > 0; }
};

inline String string(const char *sz_string) { return String{ (char*)sz_string, (i32)strlen(sz_string) }; }

struct StringBuilder {
    struct Block {
        Block *next = nullptr;
        i32 written = 0;
        char data[4096];
    };

    Allocator alloc = mem_dynamic;
    Block head = {};
    Block *current = &head;
};

bool operator!=(String lhs, String rhs);
bool operator==(String lhs, String rhs);

bool starts_with(String lhs, String rhs);
bool starts_with(const char *lhs, const char *rhs);
bool ends_with(String lhs, String rhs);

String create_string(StringBuilder *sb, Allocator mem);
String create_string(const char *str, i32 length, Allocator mem);
inline String create_string(const char *sz_str, Allocator mem) { return create_string(sz_str, (i32)strlen(sz_str), mem); }
String duplicate_string(String other, Allocator mem);
void string_copy(String *dst, String src, Allocator mem);

String to_lower(String s, Allocator mem);
char to_lower(char c);

bool is_whitespace(i32 c);
bool is_number(i32 c);

String stringf(char *buffer, i32 size, const char *fmt, ...);
String stringf(Allocator mem, const char *fmt, ...);

i32 i32_from_string(String s);
bool i32_from_string(String s, i32 *dst);
bool f32_from_string(String s, f32 *dst);

String slice(String str, i32 start, i32 end);
String slice(String str, i32 start);

i32 last_of(String str, char c);

char* sz_string(String str, Allocator mem);
char* join_path(const char *sz_root, const char *sz_filename, Allocator mem);
char* last_of(char *str, char c);

i32 utf8_length(const u16 *str, i32 utf16_len, i32 limit);
i32 utf8_length(const u16 *str, i32 utf16_len);
i32 utf8_from_utf16(u8 *dst, i32 capacity, const u16 *src, i32 length);
i32 utf8_from_utf32(u8 utf8[4], i32 utf32);

i32 utf16_length(String str);
void utf16_from_string(u16 *dst, i32 capacity, String src);

String string_from_utf16(const u16 *in_str, i32 length, Allocator mem);

i32 utf32_next(char **p, char *end);
i32 utf32_it_next(char **utf8, char *end);

i32 utf8_decr(String str, i32 i);
i32 utf8_incr(String str, i32 i);
i32 utf8_truncate(String str, i32 limit);

i32 byte_index_from_codepoint_index(String str, i32 codepoint);
i32 codepoint_index_from_byte_index(String str, i32 byte);

void reset_string_builder(StringBuilder *sb);
void append_string(StringBuilder *sb, String str);
void append_stringf(StringBuilder *sb, const char *fmt, ...);

bool parse_cmd_argument(String *args, i32 count, String name, i32 values[2]);

#if defined(_WIN32)
wchar_t* wsz_string(String str, Allocator mem);
i32 utf8_length(const wchar_t *str, i32 utf16_len);
i32 utf8_from_utf16(u8 *dst, i32 capacity, const wchar_t *src, i32 length);
String string_from_utf16(const wchar_t *in_str, i32 length, Allocator mem);
#endif // defined(WIN32)

#endif // STRING_H
