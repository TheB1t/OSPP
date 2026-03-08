#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/static_array.hpp>
#include <klibcpp/elf.hpp>
#include <klibcpp/trivial.hpp>
#include <klibcpp/bitmap.hpp>
#include <sys/kexp.hpp>
#include <mm/vmm.hpp>
#include <log.hpp>

using namespace kstd::ELF32;

using LMM = FixedBitmapAllocator<
    0xA000000,
    1 << 7,
        mm::PAGE_SIZE
>;

class Linker : public NonTransferable {
    public:
        enum class Access : uint8_t {
            N   = 0b000,
            R   = 0b001,
            W   = 0b010,
            RW  = 0b011,
            X   = 0b100,
            RX  = 0b101,
            WX  = 0b110,
            RWX = 0b111,
        };

        enum class SecKind : uint8_t {
            UNKNOWN, PROGBITS, NOBITS
        };

        struct Section {
            Access   access   = Access::N;
            SecKind  kind     = SecKind::UNKNOWN;
            uint32_t align    = 1;
            uint32_t file_off = 0;
            uint32_t reg_off  = 0;
            uint32_t size     = 0;
            uint32_t sh_flags = 0;

            Section* next     = nullptr;

            bool has_flag(uint32_t flag) const {
                return (sh_flags & flag) == flag;
            }
        };

        struct Region {
            uint32_t base    = 0;
            uint32_t size    = 0;
            Access   access  = Access::N;

            uint32_t index   = 0;
            Section* section = nullptr;

            Section* tail() {
                Section* c = section;
                if (!c)
                    return nullptr;

                while (c->next)
                    c = c->next;

                return c;
            }

            void append(Section* s) {
                if (!section) {
                    section = s;
                } else {
                    tail()->next = s;
                }

                s->reg_off = mm::align_up(size, s->align);
                size       = s->reg_off + s->size;
            }

            ~Region() {
                Section* c = section;
                while (c) {
                    Section* n = c->next;
                    delete c;
                    c = n;
                }
                section = nullptr;
            }
        };

        struct SecMap {
            Region*  region;
            Section* section;
        };

        struct Layout : public NonTransferable {
            Object* obj = nullptr;

            Region  rx;
            Region  rw;

            SecMap* map = nullptr;

            Layout() {
                rx.access = Access::RX;
                rx.index  = 0;

                rw.access = Access::RW;
                rw.index  = 1;
            }

            ~Layout() {
                map = nullptr;
            }

            void* find_symbol(const char* name) {
                if (!obj || !map || !name)
                    return nullptr;

                Symbol_t* sym = obj->find_symbol(name);
                if (!sym)
                    return nullptr;

                if (sym->shndx == 0 /* SHN_UNDEF */)
                    return nullptr;

                if (sym->shndx == 0xfff2 /* SHN_COMMON */) {
                    LOG_ERR("[linker] find_symbol: COMMON symbol %s not placed into BSS\n", name);
                    return nullptr;
                }

                if (sym->shndx >= obj->sec_hdrs_num) {
                    LOG_ERR("[linker] find_symbol: bad shndx for %s\n", name);
                    return nullptr;
                }

                SecMap& m = map[sym->shndx];
                if (!m.region || !m.section) {
                    LOG_ERR("[linker] find_symbol: section unmapped for %s (shndx=%d)\n", name, sym->shndx);
                    return nullptr;
                }

                const uint32_t base = m.region->base + m.section->reg_off;
                return reinterpret_cast<void*>(static_cast<uint32_t>(base + sym->value));
            }
        };

        kstd::StaticArray<Layout, 64> layouts;

        Linker() {
            mem_mngr.clear();
        }

        Layout* load(Object* obj);

    private:
        LMM mem_mngr;

        bool    stage0(Layout* layout, Object* obj);
        bool    stage1(Region* reg, Object* obj);
        bool    stage2(Layout* layout);
};
