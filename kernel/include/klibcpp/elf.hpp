#pragma once

#include <klibcpp/cstring.hpp>
#include <klibcpp/cstdint.hpp>

namespace kstd {
    namespace ELF32 {
        typedef		uint32_t Addr_t;
        typedef		uint32_t Off_t;
        typedef		uint16_t Half_t;
        typedef		uint32_t Word_t;

        enum : Word_t {
            PT_NULL    = 0,
            PT_LOAD    = 1,
            PT_DYNAMIC = 2,
            PT_INTERP  = 3,
            PT_NOTE    = 4,
            PT_SHLIB   = 5,
            PT_PHDR    = 6
        };

        enum : Word_t {
            SHT_NULL     = 0,
            SHT_PROGBITS = 1,
            SHT_SYMTAB   = 2,
            SHT_STRTAB   = 3,
            SHT_RELA     = 4,
            SHT_HASH     = 5,
            SHT_DYNAMIC  = 6,
            SHT_NOTE     = 7,
            SHT_NOBITS   = 8,
            SHT_REL      = 9,
            SHT_DYNSYM   = 11
        };

        enum : Half_t {
            ET_NONE = 0,
            ET_REL  = 1,
            ET_EXEC = 2,
            ET_DYN  = 3
        };

        enum : Half_t {
            EM_386    = 3,
            EM_X86_64 = 62,
            EM_RISCV  = 243
        };

        enum : uint32_t {
            R_386_NONE     = 0,
            R_386_32       = 1,
            R_386_PC32     = 2,
            R_386_GOT32    = 3,
            R_386_PLT32    = 4,
            R_386_GLOB_DAT = 6,
            R_386_JMP_SLOT = 7,
            R_386_RELATIVE = 8
        };

        enum : uint32_t {
            SHF_WRITE            = 0x1,
            SHF_ALLOC            = 0x2,
            SHF_EXECINSTR        = 0x4,
            SHF_MERGE            = 0x10,
            SHF_STRINGS          = 0x20,
            SHF_INFO_LINK        = 0x40,
            SHF_LINK_ORDER       = 0x80,
            SHF_OS_NONCONFORMING = 0x100,
            SHF_GROUP            = 0x200,
        };

        typedef struct {
            struct {
                uint8_t magic[4];
                uint8_t class_;
                uint8_t data;
                uint8_t version;
                uint8_t osabi;
                uint8_t abiversion;
                uint8_t unused[7];
            } ident;

            Half_t type;
            Half_t machine;
            Word_t version;
            Addr_t entry;
            Off_t  phoff;
            Off_t  shoff;
            Word_t flags;
            Half_t ehsize;
            Half_t phentsize;
            Half_t phnum;
            Half_t shentsize;
            Half_t shnum;
            Half_t shstrndx;
        } Header_t;

        typedef struct {
            Word_t type;
            Off_t  offset;
            Addr_t vaddr;
            Addr_t paddr;
            Word_t filesz;
            Word_t memsz;
            Word_t flags;
            Word_t align;
        } ProgramHeader_t;

        typedef struct {
            Word_t name;
            Word_t type;
            Word_t flags;
            Addr_t addr;
            Off_t  offset;
            Word_t size;
            Word_t link;
            Word_t info;
            Word_t addralign;
            Word_t entsize;
        } SectionHeader_t;

        typedef struct {
            Word_t  name;
            Addr_t  value;
            Word_t  size;
            uint8_t	info;
            uint8_t	other;
            Half_t  shndx;
        } Symbol_t;

        struct Rel_t {
            Addr_t offset;
            Word_t info;
        };
        inline uint32_t REL_SYM(uint32_t i) { return i >> 8; }
        inline uint32_t REL_TYPE(uint32_t i) { return i & 0xFF; }

        class Object {
            private:
                Header_t* hdr;

            public:
                const ProgramHeader_t* prog_hdrs;
                const SectionHeader_t* sec_hdrs;
                const uint16_t prog_hdrs_num;
                const uint16_t sec_hdrs_num;

                const SectionHeader_t* symtab;
                const SectionHeader_t* strtab;
                const SectionHeader_t* dynsym;
                const SectionHeader_t* dynstr;

                static Object* create(void* ptr) {
                    auto*          header = reinterpret_cast<Header_t*>(ptr);
                    const uint8_t* id     = header->ident.magic;

                    if (id[0] != 0x7F || id[1] != 'E' || id[2] != 'L' || id[3] != 'F')
                        return nullptr;

                    if (header->machine != EM_386)
                        return nullptr;

                    return new Object(header);
                }

                const char* section_name(uint32_t index) {
                    if (index >= sec_hdrs_num)
                        return nullptr;

                    const auto* sh     = &sec_hdrs[index];
                    const auto* strtab = header_offset<const char*>(sec_hdrs[hdr->shstrndx].offset);
                    return strtab + sh->name;
                }

                const SectionHeader_t* find_section(const char* name) {
                    for (uint32_t i = 0; i < sec_hdrs_num; i++) {
                        const char* secname = section_name(i);

                        if (secname && strcmp(secname, name) == 0)
                            return &sec_hdrs[i];
                    }

                    return nullptr;
                }

                Symbol_t* find_symbol(const char* name) {
                    Symbol_t* sym = nullptr;

                    sym = _find_symbol(name, dynsym, dynstr);
                    if (sym)
                        goto _Lfound;

                    sym = _find_symbol(name, symtab, strtab);

                _Lfound:
                    return sym;
                }

                Symbol_t* symbol_by_index(int i) {
                    if (!symtab)
                        return nullptr;

                    Symbol_t* symtab = header_offset<Symbol_t*>(this->symtab->offset);
                    return &symtab[i];
                }

                inline Header_t* header() {
                    return hdr;
                }

                template<typename T>
                inline T header_as() {
                    return reinterpret_cast<T>(hdr);
                }

                template<typename T>
                inline T header_offset(uint32_t offset) {
                    return reinterpret_cast<T>(header_as<uint8_t*>() + offset);
                }

            private:
                explicit Object(Header_t* h)
                    : hdr(h),
                      prog_hdrs(header_offset<ProgramHeader_t*>(hdr->phoff)),
                      sec_hdrs(header_offset<SectionHeader_t*>(hdr->shoff)),
                      prog_hdrs_num(hdr->phnum),
                      sec_hdrs_num(hdr->shnum),
                      symtab(find_section(".symtab")),
                      strtab(find_section(".strtab")),
                      dynsym(find_section(".dynsym")),
                      dynstr(find_section(".dynstr"))
                {}

                Symbol_t* _find_symbol(const char* name, const SectionHeader_t* sym, const SectionHeader_t* str) {
                    if (!sym || !str)
                        return nullptr;

                    const char*    strtab    = header_offset<const char*>(str->offset);
                    Symbol_t*      symtab    = header_offset<Symbol_t*>(sym->offset);

                    const uint32_t sym_count = sym->size / sizeof(Symbol_t);
                    for (uint32_t i = 0; i < sym_count; ++i) {
                        const char* symname = strtab + symtab[i].name;
                        if (symtab[i].name && strcmp(symname, name) == 0)
                            return &symtab[i];
                    }

                    return nullptr;
                }
        };
    }
}