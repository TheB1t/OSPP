#include <sys/ld.hpp>
#include <mm/pmm.hpp>
#include <klibcpp/cstring.hpp>
#include <log.hpp>

using namespace kstd::ELF32;

namespace {
    static constexpr uint16_t SHN_UNDEF  = 0x0000;
    static constexpr uint16_t SHN_ABS    = 0xfff1;
    static constexpr uint16_t SHN_COMMON = 0xfff2;

    static inline bool access_is_loadable(Linker::Access a) {
        switch (a) {
            case Linker::Access::RX:
            case Linker::Access::R:
            case Linker::Access::RW:
                return true;
            default:
                return false;
        }
    }

    static inline const char* access_name(Linker::Access a) {
        switch (a) {
            case Linker::Access::N:   return "N";
            case Linker::Access::R:   return "R";
            case Linker::Access::W:   return "W";
            case Linker::Access::RW:  return "RW";
            case Linker::Access::X:   return "X";
            case Linker::Access::RX:  return "RX";
            case Linker::Access::WX:  return "WX";
            case Linker::Access::RWX: return "RWX";
            default:                  return "?";
        }
    }

    static inline const char* seckind_name(Linker::SecKind k) {
        switch (k) {
            case Linker::SecKind::PROGBITS: return "PROGBITS";
            case Linker::SecKind::NOBITS:   return "NOBITS";
            case Linker::SecKind::UNKNOWN:  return "UNKNOWN";
            default:                        return "?";
        }
    }

    static bool validate_region_section_bounds(const Linker::Region* reg,
        const Linker::Section* sec,
        uint32_t extra = 0) {
        if (!reg || !sec)
            return false;

        if (sec->reg_off > reg->size)
            return false;

        if (sec->size > reg->size)
            return false;

        if (sec->reg_off + sec->size > reg->size)
            return false;

        if (extra && sec->reg_off + extra > reg->size)
            return false;

        return true;
    }
}

Linker::Layout* Linker::load(Object* obj) {
    kstd::InterruptSpinLockGuard guard(lock_);

    if (!obj) {
        LOG_ERR("[linker] load: null object\n");
        return nullptr;
    }

    Layout* layout = &layouts.emplace();
    layout->obj = obj;

    if (!stage0(layout, obj))
        goto _Lerror;

    if (!stage1(&layout->rx, obj))
        goto _Lerror;

    if (!stage1(&layout->rw, obj))
        goto _Lerror;

    if (!stage2(layout))
        goto _Lerror;

    return layout;

_Lerror:
    unload_locked(layout);
    return nullptr;
}

Linker::~Linker() {
    kstd::InterruptSpinLockGuard guard(lock_);

    while (!layouts.empty()) {
        for (size_t i = 0; i < layouts.capacity(); ++i) {
            Layout* layout = layouts.ptr(i);
            if (!layout)
                continue;

            unload_locked(layout);
            break;
        }
    }
}

void Linker::unload(Layout* layout) {
    kstd::InterruptSpinLockGuard guard(lock_);
    unload_locked(layout);
}

bool Linker::stage0(Layout* layout, Object* obj) {
    if (!layout || !obj) {
        LOG_ERR("[linker] stage0: invalid args\n");
        return false;
    }

    if (!obj->sec_hdrs || !obj->sec_hdrs_num) {
        LOG_ERR("[linker] stage0: object has no section headers\n");
        return false;
    }

    layout->map = new SecMap[obj->sec_hdrs_num];
    memset(reinterpret_cast<uint8_t*>(layout->map), 0, obj->sec_hdrs_num * sizeof(SecMap));

    for (uint16_t i = 0; i < obj->sec_hdrs_num; ++i) {
        const SectionHeader_t* sec = &obj->sec_hdrs[i];

        if (!(sec->flags & SHF_ALLOC))
            continue;

        if (sec->type == SHT_NULL || sec->size == 0)
            continue;

        Section* new_sec = new Section();
        new_sec->kind = (sec->type == SHT_PROGBITS) ? SecKind::PROGBITS :
            (sec->type == SHT_NOBITS)   ? SecKind::NOBITS   :
            SecKind::UNKNOWN;
        new_sec->align    = sec->addralign ? sec->addralign : 1;
        new_sec->file_off = sec->offset;
        new_sec->reg_off  = 0;
        new_sec->size     = sec->size;
        new_sec->sh_flags = sec->flags;

        if (new_sec->has_flag(SHF_EXECINSTR)) {
            new_sec->access = Access::RX;
        } else if (new_sec->has_flag(SHF_WRITE)) {
            new_sec->access = Access::RW;
        } else {
            new_sec->access = Access::R;
        }

        if (!access_is_loadable(new_sec->access)) {
            LOG_WARN("[linker] stage0: dropping non-loadable alloc section idx=%u access=%s\n",
                i, access_name(new_sec->access));
            delete new_sec;
            continue;
        }

        switch (new_sec->access) {
            case Access::RX: {
                layout->rx.append(new_sec);
                layout->map[i] = { &layout->rx, new_sec };
                break;
            }

            case Access::R:
            case Access::RW: {
                layout->rw.append(new_sec);
                layout->map[i] = { &layout->rw, new_sec };
                break;
            }

            default: {
                LOG_WARN("[linker] stage0: unexpected access class idx=%u access=%s\n",
                    i, access_name(new_sec->access));
                delete new_sec;
                continue;
            }
        }

    }

    layout->rx.size = mm::align_up(layout->rx.size, mm::PAGE_SIZE);
    layout->rw.size = mm::align_up(layout->rw.size, mm::PAGE_SIZE);

    return true;
}

bool Linker::stage1(Region* reg, Object* obj) {
    if (!reg || !obj) {
        LOG_ERR("[linker] stage1: invalid args\n");
        return false;
    }

    if (reg->size == 0) {
        return true;
    }

    const uint32_t pages = reg->size / mm::PAGE_SIZE;

    reg->base = mem_mngr.alloc_units(pages);

    uint32_t phys = mm::pmm::alloc_frames(pages);

    if (!phys) {
        mem_mngr.free_units(reg->base, pages);
        reg->base = 0;
        LOG_WARN("[linker] stage1: out of physical memory\n");
        return false;
    }

    mm::vmm::map_pages(reg->base, phys, pages, mm::Flags::Present | mm::Flags::Writable);

    for (Section* sec = reg->section; sec; sec = sec->next) {
        if (!validate_region_section_bounds(reg, sec)) {
            LOG_ERR(
                "[linker] stage1: section bounds invalid in region=%u reg_off=0x%08x size=0x%08x region_size=0x%08x\n",
                reg->index, sec->reg_off, sec->size, reg->size);
            return false;
        }

        uint8_t* dst = reinterpret_cast<uint8_t*>(reg->base + sec->reg_off);

        if (sec->kind == SecKind::PROGBITS) {
            uint8_t* src = obj->header_offset<uint8_t*>(sec->file_off);
            if (!src) {
                LOG_ERR("[linker] stage1: null src for PROGBITS file_off=0x%08x\n", sec->file_off);
                return false;
            }
            memcpy(dst, src, sec->size);
        } else if (sec->kind == SecKind::NOBITS) {
            memset(dst, 0, sec->size);
        } else {
            LOG_WARN("[linker] stage1: unknown section kind, zeroing region=%u off=0x%08x size=0x%08x\n",
                reg->index, sec->reg_off, sec->size);
            memset(dst, 0, sec->size);
        }
    }

    return true;
}

void Linker::unload_region_locked(Region& reg) {
    if (!reg.base || !reg.size)
        return;

    const uint32_t pages = reg.size / mm::PAGE_SIZE;

    mm::vmm::unmap_pages(reg.base, pages);
    mem_mngr.free_units(reg.base, pages);

    reg.base = 0;
}

void Linker::unload_locked(Layout* layout) {
    if (!layout)
        return;

    unload_region_locked(layout->rw);
    unload_region_locked(layout->rx);

    const size_t index = layouts.index_of(layout);
    if (index == kstd::StaticArray<Layout, 64>::npos) {
        LOG_ERR("[linker] unload: layout is not tracked\n");
        return;
    }

    layouts.erase(index);
}

bool Linker::stage2(Layout* layout) {
    if (!layout || !layout->obj || !layout->map) {
        LOG_ERR("[linker] stage2: invalid args\n");
        return false;
    }

    Object* obj = layout->obj;

    for (uint16_t i = 0; i < obj->sec_hdrs_num; ++i) {
        const auto& sec = obj->sec_hdrs[i];
        if (sec.type != SHT_REL)
            continue;

        const uint16_t target_idx = sec.info;
        if (target_idx >= obj->sec_hdrs_num) {
            LOG_ERR("[linker] stage2: bad target_idx=%u in rel sec=%u\n", target_idx, i);
            return false;
        }

        auto& target_map = layout->map[target_idx];
        if (!target_map.region || !target_map.section) {
            LOG_WARN("[linker] stage2: rel sec=%u target_idx=%u unmapped, skipping\n", i, target_idx);
            continue;
        }

        if (!target_map.region->base) {
            LOG_ERR("[linker] stage2: rel sec=%u target_idx=%u has zero region base\n", i, target_idx);
            return false;
        }

        Rel_t* rel = obj->header_offset<Rel_t*>(sec.offset);
        if (!rel && sec.size) {
            LOG_ERR("[linker] stage2: null relocation table for sec=%u\n", i);
            return false;
        }

        const size_t n = sec.size / sizeof(Rel_t);

        for (size_t j = 0; j < n; ++j) {
            auto&          r    = rel[j];
            const uint32_t type = REL_TYPE(r.info);
            const uint32_t symi = REL_SYM(r.info);

            Symbol_t*      sym  = obj->symbol_by_index(symi);
            if (!sym) {
                LOG_ERR("[linker] stage2: bad symbol index=%u in rel sec=%u entry=%u\n",
                    symi, i, static_cast<uint32_t>(j));
                return false;
            }

            if (r.offset + sizeof(uint32_t) > target_map.section->size) {
                LOG_ERR(
                    "[linker] stage2: relocation offset OOB relsec=%u target_idx=%u off=0x%08x target_size=0x%08x\n",
                    i, target_idx, r.offset, target_map.section->size);
                return false;
            }

            const uint32_t P = target_map.region->base +
                target_map.section->reg_off +
                r.offset;

            uint32_t A = *reinterpret_cast<uint32_t*>(P);
            uint32_t S = 0;

            switch (sym->shndx) {
                case SHN_UNDEF: {
                    const char* name = obj->header_offset<const char*>(obj->strtab->offset) + sym->name;
                    if (auto* e = kexp::lookup(name)) {
                        S = reinterpret_cast<uint32_t>(e->addr);
                    } else {
                        LOG_WARN("[linker] stage2: unresolved symbol: %s\n", name);
                        continue;
                    }
                    break;
                }

                case SHN_ABS: {
                    S = sym->value;
                    break;
                }

                case SHN_COMMON: {
                    LOG_ERR("[linker] stage2: COMMON symbol not lowered into BSS (sym=%.*s)\n",
                        32, obj->header_offset<const char*>(obj->strtab->offset) + sym->name);
                    continue;
                }

                default: {
                    if (sym->shndx >= obj->sec_hdrs_num) {
                        LOG_ERR("[linker] stage2: bad shndx=%u for sym index=%u\n",
                            sym->shndx, symi);
                        return false;
                    }

                    auto& sm = layout->map[sym->shndx];
                    if (!sm.region || !sm.section) {
                        LOG_ERR("[linker] stage2: unmapped symbol section shndx=%u symi=%u\n",
                            sym->shndx, symi);
                        return false;
                    }

                    if (!sm.region->base) {
                        LOG_ERR("[linker] stage2: symbol region base is zero shndx=%u symi=%u\n",
                            sym->shndx, symi);
                        return false;
                    }

                    S = sm.region->base + sm.section->reg_off + sym->value;
                    break;
                }
            }

            switch (type) {
                case R_386_32:
                    *reinterpret_cast<uint32_t*>(P) = S + A;
                    break;

                case R_386_PC32:
                    *reinterpret_cast<uint32_t*>(P) = S + A - P;
                    break;

                default:
                    LOG_ERR("[linker] stage2: unsupported relocation type=%u\n", type);
                    return false;
            }
        }
    }

    return true;
}
