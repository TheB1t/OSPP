#pragma once

#include <klibcpp/cstdint.hpp>
#include <multiboot.hpp>
#include <mm/pmm.hpp>

namespace multiboot {
    inline bool has_modules(const multiboot_info_t& info) {
        return TEST_MASK(info.flags, MULTIBOOT_INFO_MODS) && info.mods_count && info.mods_addr;
    }

    inline uint32_t module_count(const multiboot_info_t& info) {
        return has_modules(info) ? info.mods_count : 0;
    }

    inline const multiboot_module_t* modules(const multiboot_info_t& info) {
        if (!has_modules(info))
            return nullptr;

        return reinterpret_cast<const multiboot_module_t*>(info.mods_addr);
    }

    template<typename Fn>
    inline void for_each_module(const multiboot_info_t& info, Fn&& fn) {
        const multiboot_module_t* module = modules(info);
        if (!module)
            return;

        for (uint32_t i = 0; i < info.mods_count; ++i)
            fn(i, module[i]);
    }

    inline uint32_t max_module_end_aligned(const multiboot_info_t& info) {
        uint32_t end = 0;

        for_each_module(info, [&](uint32_t, const multiboot_module_t& module) {
                uint32_t aligned_end = mm::align_up(module.mod_end, mm::PAGE_SIZE);
                if (aligned_end > end)
                    end = aligned_end;
            });

        return end;
    }
}
