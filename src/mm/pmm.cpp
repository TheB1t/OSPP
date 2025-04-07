#include <mm/pmm.hpp>
#include <multiboot.hpp>
#include <kutils.hpp>
#include <driver/serial.hpp>

namespace mm {
    uint32_t pmm::bitmap[BITMAP_SIZE] = { 0 };
    uint32_t pmm::used_pages = 0;
    uint32_t pmm::memory_size = 0;

    static const char* typeNames[] = { "UNKNOWN", "AVAILABLE", "RESERVED", "ACPI", "NVS", "BAD MEMORY" };

    void pmm::init(multiboot_info_t* mboot) {
        if (!TEST_MASK(mboot->flags, MULTIBOOT_INFO_MEM_MAP))
            kutils::panic("Bootloader can't store memory map!\n");

        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mboot->mmap_addr;
        uint32_t mmap_end = (uint32_t)mboot->mmap_addr + mboot->mmap_length;
        memory_size = 0;

        for (uint32_t i = 0; i < BITMAP_SIZE; i++)
            bitmap[i] = 0xFFFFFFFF;

        serial::printf("[pmm] Memory map provided by bootloader:\n");
        while ((uint32_t)mmap < mmap_end) {
            uint32_t base = mmap->addr;
            uint32_t length = mmap->len;
            uint32_t type = mmap->type;

            serial::printf("   0x%08x - 0x%08x (len 0x%08x) [%s]\n", 
                base, base + length, length, typeNames[type]
            );

            if (type == MULTIBOOT_MEMORY_AVAILABLE && base >= 0x100000) {
                memory_size += length;
                init_region(base, length);
            }

            mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }

        // Map kernel memory
        serial::printf("[pmm] Mapping kernel memory\n");
        serial::printf("- from 0x%08x\n", (uint32_t)&__kernel_start);
        serial::printf("- to 0x%08x\n", (uint32_t)&__kernel_end);
        deinit_region((uint32_t)&__kernel_start, (uint32_t)&__kernel_end - (uint32_t)&__kernel_start);
        used_pages += ((uint32_t)&__kernel_end - (uint32_t)&__kernel_start) / PAGE_SIZE;

        uint32_t tmem = total_memory();
        uint32_t fmem = free_memory();
        uint32_t umem = used_memory();

        using namespace kutils::ByteConverter;

        Unit tmemu = bestUnit(tmem);
        Unit fmemu = bestUnit(fmem);
        Unit umemu = bestUnit(umem);

        serial::printf("[pmm] Total memory: %.2f %s (%d bytes)\n", convert(tmem, Unit::B, tmemu), unitNames[tmemu], tmem);
        serial::printf("[pmm] Free Memory: %.2f %s (%d bytes)\n", convert(fmem, Unit::B, fmemu), unitNames[fmemu], fmem);
        serial::printf("[pmm] Used Memory: %.2f %s (%d bytes)\n", convert(umem, Unit::B, umemu), unitNames[umemu], umem);
    }

    uint32_t pmm::alloc_page() {
        for (uint32_t i = 0; i < MAX_PAGES; i++) {
            if (!test_bit(i)) {
                set_bit(i);
                used_pages++;
                return i * PAGE_SIZE;
            }
        }
        kutils::panic("Out of physical memory (alloc_page)");
        return 0; // This will never be reached
    }

    void pmm::free_page(uint32_t addr) {
        uint32_t bit = addr / PAGE_SIZE;
        if (bit >= MAX_PAGES)
            kutils::panic("Invalid page free (free_page)");

        if (!test_bit(bit))
            kutils::panic("Double free detected (free_page)");

        clear_bit(bit);
        used_pages--;
    }

    uint32_t pmm::alloc_pages(uint32_t count) {
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

                used_pages += count;
                return i * PAGE_SIZE;
            }
        }

        kutils::panic("Out of physical memory (alloc_pages)");
        return 0; // Unreachable
    }

    void pmm::free_pages(uint32_t base, uint32_t count) {
        if (count == 0)
            return;

        uint32_t start = base / PAGE_SIZE;

        if (start + count > MAX_PAGES)
            kutils::panic("Invalid range in free_pages (free_pages)");

        for (uint32_t i = 0; i < count; i++) {
            if (!test_bit(start + i))
                kutils::panic("Double free in free_pages (free_pages)");

            clear_bit(start + i);
        }

        used_pages -= count;
    }

    void pmm::set_bit(uint32_t bit) {
        bitmap[bit / 32] |= (1 << (bit % 32));
    }

    void pmm::clear_bit(uint32_t bit) {
        bitmap[bit / 32] &= ~(1 << (bit % 32));
    }

    bool pmm::test_bit(uint32_t bit) {
        return bitmap[bit / 32] & (1 << (bit % 32));
    }

    void pmm::init_region(uint32_t base, uint32_t size) {
        if (base > UINT32_MAX)
            return;

        if (base + size > UINT32_MAX)
            size = UINT32_MAX - base;

        uint32_t align = base % PAGE_SIZE;
        if (align) {
            size -= align;
            base += align;
        }

        if (size < PAGE_SIZE)
            return;

        size -= size % PAGE_SIZE;
        uint32_t start = base / PAGE_SIZE;
        uint32_t pages = size / PAGE_SIZE;

        for (uint32_t i = start; i < start + pages; i++) {
            if (i >= MAX_PAGES)
                break;

            clear_bit(i);
        }
    }

    void pmm::deinit_region(uint32_t base, size_t size) {
        uint32_t start = base / PAGE_SIZE;
        uint32_t pages = size / PAGE_SIZE;

        for (uint32_t i = start; i < start + pages; i++) {
            set_bit(i);
        }
    }

    uint32_t pmm::free_memory() {
        return (memory_size / PAGE_SIZE - used_pages) * PAGE_SIZE;
    }

    uint32_t pmm::used_memory() {
        return used_pages * PAGE_SIZE;
    }

    uint32_t pmm::total_memory() {
        return memory_size;
    }
}