#include <mm/pmm.hpp>
#include <multiboot.hpp>
#include <klibcpp/kstd.hpp>
#include <log.hpp>

namespace mm {
    PMM                pmm::mem_mngr;
    uint32_t           pmm::available_frames;
    uint32_t           pmm::used_frames;

    static const char* typeNames[] = { "UNKNOWN", "AVAILABLE", "RESERVED", "ACPI", "NVS", "BAD MEMORY" };

    static uint32_t page_aligned_span(uint32_t base, uint32_t size) {
        if (!size)
            return 0;

        uint32_t start = align_down(base, PAGE_SIZE);
        uint32_t end   = align_up(base + size, PAGE_SIZE);
        return end - start;
    }

    void pmm::init(multiboot_info_t* mboot) {
        if (!TEST_MASK(mboot->flags, MULTIBOOT_INFO_MEM_MAP))
            kstd::panic("Bootloader can't store memory map!\n");

        multiboot_memory_map_t* mmap     = (multiboot_memory_map_t*)mboot->mmap_addr;
        uint32_t                mmap_end = (uint32_t)mboot->mmap_addr + mboot->mmap_length;
        available_frames = 0;
        used_frames      = 0;

        mem_mngr.mark_units_used(0, MAX_FRAMES);


        uint32_t memory_size = 0;
        LOG_INFO("[pmm] Memory map provided by bootloader:\n");
        while ((uint32_t)mmap < mmap_end) {
            uint32_t base   = mmap->addr;
            uint32_t length = mmap->len;
            uint32_t type   = mmap->type;

            LOG_INFO("[pmm] 0x%08x - 0x%08x (len 0x%08x) [%s]\n",
                base, base + length, length, typeNames[type]
            );

            // Skip first 1MB of memory for not occupy it later
            if (type == MULTIBOOT_MEMORY_AVAILABLE && base >= 0x100000) {
                memory_size += length;
                init_region(base, length);
            }

            mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }

        available_frames = (memory_size + 0x100000) / PAGE_SIZE;

        uint32_t low_boot_reserved_end = align_up((uint32_t)&__kernel_end, PAGE_SIZE);

        if (TEST_MASK(mboot->flags, MULTIBOOT_INFO_MODS)) {
            multiboot_module_t* mod = reinterpret_cast<multiboot_module_t*>(mboot->mods_addr);
            for (uint32_t i = 0; i < mboot->mods_count; ++i) {
                uint32_t mod_end = align_up(mod[i].mod_end, PAGE_SIZE);
                if (mod_end > low_boot_reserved_end)
                    low_boot_reserved_end = mod_end;
            }
        }

        LOG_INFO("[pmm] Reserving kernel image 0x%08x-0x%08x\n", (uint32_t)&__kernel_start, (uint32_t)&__kernel_end);
        uint32_t kernel_span = page_aligned_span((uint32_t)&__kernel_start,
                (uint32_t)&__kernel_end - (uint32_t)&__kernel_start);
        deinit_region((uint32_t)&__kernel_start, kernel_span);
        used_frames += ((uint32_t)&__kernel_end - (uint32_t)&__kernel_start) / PAGE_SIZE;

        if (TEST_MASK(mboot->flags, MULTIBOOT_INFO_MODS)) {
            multiboot_module_t* mod = reinterpret_cast<multiboot_module_t*>(mboot->mods_addr);
            for (uint32_t i = 0; i < mboot->mods_count; ++i) {
                uint32_t size = mod[i].mod_end - mod[i].mod_start;
                uint32_t span = page_aligned_span(mod[i].mod_start, size);

                LOG_INFO("[pmm] Reserving module %d at 0x%08x-0x%08x\n", i, mod[i].mod_start, mod[i].mod_end);
                deinit_region(mod[i].mod_start, span);
            }
        }

        // Keep the entire low bootstrap area out of the allocator. This covers the
        // kernel image, multiboot modules and other early low-memory objects so PMM
        // cannot hand out frames that alias live code/data.
        if (low_boot_reserved_end > 0x00100000) {
            LOG_INFO("[pmm] Reserving low bootstrap area 0x%08x-0x%08x\n", 0x00100000, low_boot_reserved_end);
            deinit_region(0x00100000, low_boot_reserved_end - 0x00100000);
        }

        uint32_t tmem = total_memory();
        uint32_t fmem = free_memory();
        uint32_t umem = used_memory();

        using namespace kstd::ByteConverter;

        Unit tmemu = bestUnit(tmem);
        Unit fmemu = bestUnit(fmem);
        Unit umemu = bestUnit(umem);

        LOG_INFO("[pmm] Total memory: %.2f %s (%d bytes)\n", convert(tmem, Unit::B, tmemu), unitNames[tmemu], tmem);
        LOG_INFO("[pmm] Free Memory: %.2f %s (%d bytes)\n", convert(fmem, Unit::B, fmemu), unitNames[fmemu], fmem);
        LOG_INFO("[pmm] Used Memory: %.2f %s (%d bytes)\n", convert(umem, Unit::B, umemu), unitNames[umemu], umem);
    }

    uint32_t pmm::alloc_frames(uint32_t count) {
        uint32_t addr = mem_mngr.alloc_units(count);
        used_frames += count;
        return addr;
    }

    void pmm::free_frames(uint32_t base, uint32_t count) {
        mem_mngr.free_units(base, count);
        used_frames -= count;
    }

    uint32_t pmm::alloc_frame() {
        return alloc_frames(1);
    }

    void pmm::free_frame(uint32_t addr) {
        free_frames(addr, 1);
    }

    void pmm::init_region(uint32_t base, uint32_t size) {
        if (base > UINT32_MAX)
            return;

        if (base + size > UINT32_MAX)
            size = UINT32_MAX - base;

        uint32_t align = base % PAGE_SIZE;
        if (align) {
            uint32_t delta = PAGE_SIZE - align;
            if (size <= delta)
                return;

            size -= delta;
            base += delta;
        }

        if (size < PAGE_SIZE)
            return;

        size -= size % PAGE_SIZE;

        uint32_t frames = size / PAGE_SIZE;
        if ((base / PAGE_SIZE) >= MAX_FRAMES)
            return;

        if ((base / PAGE_SIZE) + frames > MAX_FRAMES)
            frames = MAX_FRAMES - (base / PAGE_SIZE);

        mem_mngr.mark_units_free(base, frames);
    }

    void pmm::deinit_region(uint32_t base, size_t size) {
        if (!size)
            return;

        uint32_t aligned_base = align_down(base, PAGE_SIZE);
        uint32_t end          = align_up(base + size, PAGE_SIZE);

        if ((aligned_base / PAGE_SIZE) >= MAX_FRAMES)
            return;

        uint32_t frames = (end - aligned_base) / PAGE_SIZE;
        if ((aligned_base / PAGE_SIZE) + frames > MAX_FRAMES)
            frames = MAX_FRAMES - (aligned_base / PAGE_SIZE);

        mem_mngr.mark_units_used(aligned_base, frames);
    }

    uint32_t pmm::free_memory() {
        return (available_frames - used_frames) * PAGE_SIZE;
    }

    uint32_t pmm::used_memory() {
        return used_frames * PAGE_SIZE;
    }

    uint32_t pmm::total_memory() {
        return available_frames * PAGE_SIZE;
    }
}
