/*
    This file contains implementation of
    OS++ physical memory manager.
*/
#pragma once

#include <klibcpp/cstdint.hpp>
#include <multiboot.hpp>

__extern_asm symbol __kernel_end;
__extern_asm symbol __kernel_start;

namespace mm {
    static constexpr uint32_t PAGE_SIZE = 4096;
    static constexpr uint32_t MAX_PAGES = 1 << 20;

    class pmm {
        public:
            static constexpr uint32_t BITMAP_SIZE = (MAX_PAGES + 31) / 32;

            static void init(multiboot_info_t* mboot);
            static uint32_t alloc_page();
            static void free_page(uint32_t addr);
            static uint32_t alloc_pages(uint32_t count);
            static void free_pages(uint32_t base, uint32_t count);
            static uint32_t free_memory();
            static uint32_t used_memory();
            static uint32_t total_memory();

        private:
            static uint32_t bitmap[BITMAP_SIZE];
            static uint32_t used_pages;
            static uint32_t memory_size;
            
            static void set_bit(uint32_t bit);
            static void clear_bit(uint32_t bit);
            static bool test_bit(uint32_t bit);
            static void init_region(uint32_t base, size_t size);
            static void deinit_region(uint32_t base, size_t size);
    };
};