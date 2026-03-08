/*
    This file contains implementation of
    OS++ physical memory manager.
*/
#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/bitmap.hpp>
#include <multiboot.hpp>

__extern_asm symbol __kernel_end;
__extern_asm symbol __kernel_start;

namespace mm {
    static constexpr uint32_t PAGE_SIZE  = 4096;
    static constexpr uint32_t MAX_FRAMES = 1 << 20;

    using PMM = FixedBitmapAllocator<
        0x00000000,
        MAX_FRAMES,
        mm::PAGE_SIZE
    >;

    constexpr inline uint32_t align_down(uint32_t val, uint32_t align) {
        return val & ~(align - 1);
    }

    constexpr inline uint32_t align_up(uint32_t val, uint32_t align) {
        return (val + align - 1) & ~(align - 1);
    }

    class pmm {
        public:
            static void     init(multiboot_info_t* mboot);
            static uint32_t alloc_frame();
            static void     free_frame(uint32_t addr);
            static uint32_t alloc_frames(uint32_t count);
            static void     free_frames(uint32_t base, uint32_t count);
            static uint32_t free_memory();
            static uint32_t used_memory();
            static uint32_t total_memory();

        private:
            static PMM mem_mngr;

            static uint32_t available_frames;
            static uint32_t used_frames;

            static void     init_region(uint32_t base, size_t size);
            static void     deinit_region(uint32_t base, size_t size);
    };
};
