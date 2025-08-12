#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/vector.hpp>
#include <klibcpp/elf.hpp>
#include <sys/kexp.hpp>
#include <mm/vmm.hpp>
#include <log.hpp>

using namespace kstd::ELF32;

class lmm {
    public:
        static constexpr uint32_t BASE_ADDR = 0xA0000000;
        static constexpr uint32_t MAX_PAGES = 1 << 8;
        static constexpr uint32_t BITMAP_SIZE = (MAX_PAGES + 31) / 32;
        static constexpr uint32_t PAGE_SIZE = mm::PAGE_SIZE;

        lmm() {
            bitmap = new uint32_t[BITMAP_SIZE];
            memset((uint8_t*)bitmap, 0, BITMAP_SIZE * sizeof(uint32_t));
        }

        ~lmm() {
            delete [] bitmap;
        }

        uint32_t alloc_pages(uint32_t count) {
            if (count == 0)
                return 0;

            for (uint32_t i = 0; i <= MAX_PAGES - count; i++) {
                bool found = true;
                for (uint32_t j = 0; j < count; j++) {
                    if (test_bit(i + j)) {
                        found = false;
                        i += j; // Skip ahead to avoid checking overlapping ranges
                        break;
                    }
                }

                if (found) {
                    for (uint32_t j = 0; j < count; j++)
                        set_bit(i + j);

                    return (i * PAGE_SIZE) + BASE_ADDR;
                }
            }

            kstd::panic("Out of memory (alloc_pages)");
            return 0; // Unreachable
        }

        void free_pages(uint32_t base, uint32_t count) {
            if (count == 0)
                return;

            uint32_t start = (base - BASE_ADDR) / PAGE_SIZE;

            if (start + count > MAX_PAGES)
                kstd::panic("Invalid range (free_pages)");

            for (uint32_t i = 0; i < count; i++) {
                if (!test_bit(start + i))
                    kstd::panic("float free (free_pages)");

                clear_bit(start + i);
            }
        }

        uint32_t alloc_page() {
            return alloc_pages(1);
        }

        void free_page(uint32_t addr) {
            free_pages(addr, 1);
        }

    private:
        uint32_t* bitmap;

        void set_bit(uint32_t bit) {
            bitmap[bit / 32] |= (1 << (bit % 32));
        }

        void clear_bit(uint32_t bit) {
            bitmap[bit / 32] &= ~(1 << (bit % 32));
        }

        bool test_bit(uint32_t bit) {
            return bitmap[bit / 32] & (1 << (bit % 32));
        }
};

class Linker {
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
            Access   access;
            SecKind  kind;
            uint32_t align;
            uint32_t file_off;
            uint32_t reg_off; // offset inside region
            uint32_t size;
            uint32_t sh_flags;

            Section* next = nullptr;

            inline bool has_flag(uint32_t flag) {
                return (sh_flags & flag) == flag;
            }
        };

        struct Region {
            uint32_t base;
            uint32_t size;  // Size in bytes
            Access access;  // Overall pages access, all sections should have same access

            uint32_t index;
            Section* section; // Head

            inline Section* tail() {
                Section* c = section;
                if (!c)
                    return nullptr;

                while (c->next)
                    c = c->next;

                return c;
            }

            inline void append(Section* s) {
                if (!section) {
                    section = s;
                } else {
                    tail()->next = s;
                }

                s->reg_off = mm::align_up(size, s->align);
                size = s->reg_off + s->size;
            }

            ~Region() {
                Section* c = section;
                while (c) {
                    Section* c1 = c->next;
                    delete c;
                    c = c1;
                }

                section = nullptr;
            }
        };

        struct SecMap {
            Region*  region;
            Section* section;
        };

        struct Layout {
            Object* obj;
            // .text*
            Region rx;
            // .data* + .bss* + .rodata* + .init/.fini
            Region rw;

            SecMap* map;

            void* find_symbol(const char* name) {
                if (!obj || !map || !name)
                    return nullptr;

                Symbol_t* sym = obj->find_symbol(name);
                if (!sym)
                    return nullptr;

                if (sym->shndx == 0 /* SHN_UNDEF */)
                    return nullptr;

                if (sym->shndx == 0xfff2 /* SHN_COMMON */) {
                    LOG_ERR("find_symbol: COMMON symbol %s not placed into BSS\n", name);
                    return nullptr;
                }

                if (sym->shndx >= obj->sec_hdrs_num) {
                    LOG_ERR("find_symbol: bad shndx for %s\n", name);
                    return nullptr;
                }

                SecMap& m = map[sym->shndx];
                if (!m.region || !m.section) {
                    LOG_ERR("find_symbol: section unmapped for %s (shndx=%d)\n", name, sym->shndx);
                    return nullptr;
                }

                const uint32_t base = m.region->base + m.section->reg_off;
                return reinterpret_cast<void*>(static_cast<uint32_t>(base + sym->value));
            }
        };

        kstd::vector<Layout*> layouts;

        static Linker* get() {
            static Linker* instance = new Linker();
            return instance;
        }

        Linker(const Linker&) = delete;
        Linker& operator=(const Linker&) = delete;

        Layout* load(Object* obj) {
            Layout* layout = new Layout();

            memset((uint8_t*)layout, 0, sizeof(Layout));

            layout->rx = { .base = 0, .size=0, .access=Access::RX, .index=0, .section=nullptr };
            layout->rw = { .base = 0, .size=0, .access=Access::RW, .index=1, .section=nullptr };

            layout->obj = obj;

            if (!stage0(layout, obj))
                goto _Lerror;

            if (!stage1(&layout->rx, obj))
                goto _Lerror;

            if (!stage1(&layout->rw, obj))
                goto _Lerror;

            if (!stage2(layout))
                goto _Lerror;

            layouts.push_back(layout);
            return layout;

        _Lerror:
            if (layout) {
                if (layout->map)
                    delete [] layout->map;
                delete layout;
            }

            return nullptr;
        }

    private:
        lmm mem_mngr;

        Linker() = default;

        bool stage0(Layout* layout, Object* obj) {
            layout->map = new SecMap[obj->sec_hdrs_num];
            memset((uint8_t*)layout->map, 0, obj->sec_hdrs_num * sizeof(SecMap));

            for (uint16_t i = 0; i < obj->sec_hdrs_num; i++) {
                const SectionHeader_t* sec = &obj->sec_hdrs[i];

                if (!(sec->flags & SHF_ALLOC))
                    continue;

                if (sec->type == SHT_NULL || sec->size == 0)
                    continue;

                Section* new_sec = new Section {
                    .access = Access::N,
                    .kind  = sec->type == SHT_PROGBITS ? SecKind::PROGBITS :
                             sec->type == SHT_NOBITS ? SecKind::NOBITS :
                             SecKind::UNKNOWN,
                    .align    = sec->addralign ? sec->addralign : 1,
                    .file_off = sec->offset,
                    .reg_off  = 0,
                    .size     = sec->size,
                    .sh_flags = sec->flags
                };

                if (new_sec->has_flag(SHF_EXECINSTR)) {
                    // .text*
                    new_sec->access = Access::RX;
                } else if (new_sec->has_flag(SHF_WRITE)) {
                    new_sec->access = Access::RW;
                    // if (new_sec->kind == SecKind::PROGBITS) {
                    //     // .data*
                    // } else if (new_sec->kind == SecKind::NOBITS) {
                    //     // .bss*
                    // } else {
                    //     // ?????
                    // }
                } else {
                    // .rodata*
                    new_sec->access = Access::R;
                }

                switch (new_sec->access) {
                    case Access::RX: {
                        layout->rx.append(new_sec);
                        layout->map[i] = { &layout->rx, new_sec };
                    } break;

                    case Access::R:
                    case Access::RW: {
                        layout->rw.append(new_sec);
                        layout->map[i] = { &layout->rw, new_sec };
                    } break;

                    default: {
                        delete new_sec;
                        continue;
                    };
                }
            }

            // Finalize
            layout->rx.size = mm::align_up(layout->rx.size, lmm::PAGE_SIZE);
            layout->rw.size = mm::align_up(layout->rw.size, lmm::PAGE_SIZE);

            return true;
        }

        bool stage1(Region* reg, Object* obj) {
            if (reg->size) {
                uint32_t pages = reg->size / lmm::PAGE_SIZE;
                reg->base = mem_mngr.alloc_pages(pages);

                uint32_t phys = mm::pmm::alloc_pages(pages);
                if (!phys) {
                    LOG_WARN("Can't allocate region, out of memory\n");
                    return false;
                }

                mm::vmm::map_pages(reg->base, phys, pages, mm::Flags::Present | mm::Flags::Writable);
            }

            Section* sec = reg->section;
            while (sec) {
                uint8_t* dst = reinterpret_cast<uint8_t*>(reg->base + sec->reg_off);
                if (sec->kind == SecKind::PROGBITS && sec->size) {
                    uint8_t* src = obj->header_offset<uint8_t*>(sec->file_off);
                    memcpy(dst, src, sec->size);
                } else if (sec->kind == SecKind::NOBITS && sec->size) {
                    memset(dst, 0, sec->size);
                } else {
                    // UNKNOWN/size=0
                }
                sec = sec->next;
            }

            return true;
        }

        bool stage2(Layout* layout) {
            Object* obj = layout->obj;
            for (uint16_t i = 0; i < obj->sec_hdrs_num; i++) {
                const auto& sec = obj->sec_hdrs[i];
                if (sec.type != SHT_REL)
                    continue;

                uint16_t target_idx = sec.info;
                auto& target_map = layout->map[target_idx];
                if (!target_map.region)
                    continue;

                Rel_t* rel = obj->header_offset<Rel_t*>(sec.offset);
                size_t n = sec.size / sizeof(Rel_t);

                for (size_t j = 0; j < n; ++j) {
                    auto& r = rel[j];
                    uint32_t type = REL_TYPE(r.info);
                    uint32_t symi = REL_SYM(r.info);

                    Symbol_t* sym = obj->symbol_by_index(symi);
                    uint32_t P = target_map.region->base +
                                target_map.section->reg_off +
                                r.offset;
                    uint32_t A = *reinterpret_cast<uint32_t*>(P);
                    uint32_t S = 0;

                    switch (sym->shndx) {
                        case 0: {
                            const char* name = obj->header_offset<const char*>(obj->strtab->offset) + sym->name;
                            if (auto* e = kexp::lookup(name)) {
                                S = reinterpret_cast<uint32_t>(e->addr);
                            } else {
                                LOG_WARN("Unresolved symbol: %s\n", name);
                                continue;
                            }
                            break;
                        }
                        case 0xfff1: {
                            S = sym->value;
                            break;
                        }
                        case 0xfff2: {
                            LOG_ERR("COMMON symbol should have been placed into .bss earlier (sym=%.*s)\n",
                                    /*len=*/32, obj->header_offset<const char*>(obj->strtab->offset)+sym->name);
                            continue;
                        }
                        default: {
                            if (sym->shndx >= obj->sec_hdrs_num) {
                                LOG_ERR("Bad shndx=%d\n", sym->shndx);
                                continue;
                            }
                            auto& sm = layout->map[sym->shndx];
                            if (!sm.region || !sm.section) {
                                LOG_ERR("Section unmapped for symbol shndx=%d\n", sym->shndx);
                                continue;
                            }
                            S = sm.region->base + sm.section->reg_off + sym->value;
                            break;
                        }
                    }

                    switch (type) {
                        case R_386_32:   *reinterpret_cast<uint32_t*>(P) = S + A; break;
                        case R_386_PC32: *reinterpret_cast<uint32_t*>(P) = S + A - P; break;
                        default: kstd::panic("Unsupported reloc %d", type);
                    }
                }
            }
            return true;
        }
};