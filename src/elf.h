#ifndef ELF_H
#define ELF_H

enum ElfBits {
    ELF_64 = 2,
};

enum ElfABI {
    ELF_ABI_SYSV = 0,
};

enum ElfEndian {
    ELF_LE = 1,
    ELF_BE = 2,
};

enum ElfType {
    ELF_TYPE_NONE = 0x00,
    ELF_TYPE_REL  = 0x01,
    ELF_TYPE_EXEC = 0x02,
    ELF_TYPE_DYN  = 0x03,
    ELF_TYPE_CORE = 0x04,
};

enum ElfMachine {
    ELF_MACHINE_X86_64 = 0x3e,
};

enum ElfProgramType {
    ELF_PT_NULL    = 0x00,
    ELF_PT_LOAD    = 0x01,
    ELF_PT_DYNAMIC = 0x02,
    ELF_PT_INTERP  = 0x03,
    ELF_PT_NOTE    = 0x04,
    ELF_PT_SHLIB   = 0x05,
    ELF_PT_PHDR    = 0x06,
    ELF_PT_TLS     = 0x07,

    ELF_PT_LOOS    = 0x60000000,
    ELF_PT_HIOS    = 0x6fffffff,

    ELF_PT_LOPROC  = 0x70000000,
    ELF_PT_HIPROC  = 0x7fffffff,
};

enum ElfProgramFlags {
    ELF_PF_X = 0x01,
    ELF_PF_W = 0x02,
    ELF_PF_R = 0x04,
};

enum ElfSectionType {
    ELF_SHT_NULL          = 0x00,
    ELF_SHT_PROGBITS      = 0x01,
    ELF_SHT_SYMTAB        = 0x02,
    ELF_SHT_STRTAB        = 0x03,
    ELF_SHT_RELA          = 0x04,
    ELF_SHT_HASH          = 0x05,
    ELF_SHT_DYNAMIC       = 0x06,
    ELF_SHT_NOTE          = 0x07,
    ELF_SHT_NOBITS        = 0x08,
    ELF_SHT_REL           = 0x09,
    ELF_SHT_SHLIB         = 0x0A,
    ELF_SHT_DYNSYM        = 0x0B,
    ELF_SHT_INIT_ARRAY    = 0x0E,
    ELF_SHT_FINI_ARRAY    = 0x0F,
    ELF_SHT_PREINIT_ARRAY = 0x10,
    ELF_SHT_GROUP         = 0x11,
    ELF_SHT_SYMTAB_SHNDX  = 0x12,
    ELF_SHT_NUM           = 0x13,

    ELF_SHT_LOOS          = 0x60000000,
    ELF_SHT_HIOS          = 0x6fffffff,

    ELF_SHT_LOPROC        = 0x70000000,
    ELF_SHT_HIPROC        = 0x7fffffff,
};

enum ElfSectionFlags {
    ELF_SHF_WRITE            = 0x01,
    ELF_SHF_ALLOC            = 0x02,
    ELF_SHF_EXECINSTR        = 0x04,
    ELF_SHF_MERGE            = 0x10,
    ELF_SHF_STRINGS          = 0x20,
    ELF_SHF_INFO_LINK        = 0x40,
    ELF_SHF_LINK_ORDER       = 0x80,
    ELF_SHF_OS_NONCONFORMING = 0x100,
    ELF_SHF_GROUP            = 0x200,
    ELF_SHF_TLS              = 0x400,

    ELF_SHF_MASKOS           = 0x0ff00000,
    ELF_SHF_MASKPROC         = 0xf0000000,
    ELF_SHF_ORDERED          = 0x40000000,

    ELF_SHF_EXCLUDE          = 0x80000000,
};

struct ElfHeader {
    u8  magic[4] = { 0x7f, 'E', 'L', 'F' };
    u8  bitness = ELF_64;
    u8  endian;
    u8  version = 1;
    u8  abi;
    u8  abi_version;
    u8  padding[7];
    u16 type;
    u16 machine;
    u32 version2 = 1;
    u64 entry;
    u64 phoff;
    u64 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
};

struct ElfProgramHeader {
    u32 type;
    u32 flags;
    u64 offset;
    u64 vaddr;
    u64 paddr;
    u64 filesz;
    u64 memsz;
    u64 align;
};

struct ElfSectionHeader {
    u32 name;
    u32 type;
    u64 flags;
    u64 addr;
    u64 offset;
    u64 size;
    u32 link;
    u32 info;
    u64 addralign;
    u64 entsize;
};

struct Elf64 {
    ElfHeader header;
    ElfProgramHeader *program_headers;
    ElfSectionHeader *section_headers;

    char *section_names;
    char *string_table;
    char *symbol_table;
    char *relocation_table;
};

#endif // ELF_H
