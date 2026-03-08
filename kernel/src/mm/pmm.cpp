#include <mm/pmm.hpp>
#include <mm/layout.hpp>
#include <multiboot.hpp>
#include <multiboot_utils.hpp>
#include <klibcpp/kstd.hpp>
#include <log.hpp>

namespace mm {
    PMM                pmm::mem_mngr;
    kstd::SpinLock     pmm::lock;
    uint32_t           pmm::available_frames;
    uint32_t           pmm::used_frames;

    static const char* typeNames[] = { "UNKNOWN", "AVAILABLE", "RESERVED", "ACPI", "NVS", "BAD MEMORY" };

    struct PageRange {
        uint32_t base   = 0;
        uint32_t frames = 0;

        uint32_t size() const {
            return frames * PAGE_SIZE;
        }

        bool empty() const {
            return frames == 0;
        }
    };

    static PageRange freeable_page_range(uint32_t base, uint32_t size, uint32_t min_base = 0) {
        if (!size)
            return {};

        uint64_t start = base;
        uint64_t end   = static_cast<uint64_t>(base) + size;
        uint64_t floor = min_base;
        uint64_t ceil  = static_cast<uint64_t>(MAX_FRAMES) * PAGE_SIZE;

        if (end <= floor || start >= ceil)
            return {};

        if (start < floor)
            start = floor;
        if (end > ceil)
            end = ceil;

        start = (start + PAGE_SIZE - 1) & ~(static_cast<uint64_t>(PAGE_SIZE) - 1);
        end  &= ~(static_cast<uint64_t>(PAGE_SIZE) - 1);

        if (end <= start)
            return {};

        return {
            static_cast<uint32_t>(start),
            static_cast<uint32_t>((end - start) / PAGE_SIZE)
        };
    }

    static PageRange covered_page_range(uint32_t base, uint32_t size) {
        if (!size)
            return {};

        uint64_t start = align_down(base, PAGE_SIZE);
        uint64_t end   = align_up(base + size, PAGE_SIZE);
        uint64_t ceil  = static_cast<uint64_t>(MAX_FRAMES) * PAGE_SIZE;

        if (start >= ceil)
            return {};

        if (end > ceil)
            end = ceil;

        if (end <= start)
            return {};

        return {
            static_cast<uint32_t>(start),
            static_cast<uint32_t>((end - start) / PAGE_SIZE)
        };
    }

    void pmm::init(multiboot_info_t* mboot) {
        if (!TEST_MASK(mboot->flags, MULTIBOOT_INFO_MEM_MAP))
            kstd::panic("Bootloader can't store memory map!\n");

        multiboot_memory_map_t* mmap     = (multiboot_memory_map_t*)mboot->mmap_addr;
        uint32_t                mmap_end = (uint32_t)mboot->mmap_addr + mboot->mmap_length;
        available_frames = 0;
        used_frames      = 0;

        mem_mngr.mark_units_used(0, MAX_FRAMES);

        LOG_INFO("[pmm] Memory map provided by bootloader:\n");
        while ((uint32_t)mmap < mmap_end) {
            uint32_t base   = mmap->addr;
            uint32_t length = mmap->len;
            uint32_t type   = mmap->type;

            LOG_INFO("[pmm] 0x%08x - 0x%08x (len 0x%08x) [%s]\n",
                base, base + length, length, typeNames[type]
            );

            if (type == MULTIBOOT_MEMORY_AVAILABLE) {
                PageRange range = freeable_page_range(base, length, layout::boot::LOW_USABLE_BASE);
                if (!range.empty()) {
                    available_frames += range.frames;
                    init_region(range.base, range.size());
                }
            }

            mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }

        uint32_t       low_boot_reserved_end = align_up((uint32_t)&__kernel_end, PAGE_SIZE);

        const uint32_t modules_end           = multiboot::max_module_end_aligned(*mboot);
        if (modules_end > low_boot_reserved_end)
            low_boot_reserved_end = modules_end;

        LOG_INFO("[pmm] Reserving kernel image 0x%08x-0x%08x\n", (uint32_t)&__kernel_start, (uint32_t)&__kernel_end);
        uint32_t kernel_span = covered_page_range((uint32_t)&__kernel_start,
                (uint32_t)&__kernel_end - (uint32_t)&__kernel_start).size();
        deinit_region((uint32_t)&__kernel_start, kernel_span);

        multiboot::for_each_module(*mboot, [&](uint32_t index, const multiboot_module_t& module) {
                uint32_t size = module.mod_end - module.mod_start;
                uint32_t span = covered_page_range(module.mod_start, size).size();

                LOG_INFO("[pmm] Reserving module %d at 0x%08x-0x%08x\n", index, module.mod_start, module.mod_end);
                deinit_region(module.mod_start, span);
            });

        // Keep the entire low bootstrap area out of the allocator. This covers the
        // kernel image, multiboot modules and other early low-memory objects so PMM
        // cannot hand out frames that alias live code/data.
        if (low_boot_reserved_end > layout::boot::LOW_USABLE_BASE) {
            LOG_INFO("[pmm] Reserving low bootstrap area 0x%08x-0x%08x\n",
                layout::boot::LOW_USABLE_BASE, low_boot_reserved_end);
            deinit_region(layout::boot::LOW_USABLE_BASE, low_boot_reserved_end - layout::boot::LOW_USABLE_BASE);
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
        kstd::SpinLockGuard guard(lock);
        uint32_t            addr = mem_mngr.alloc_units(count);
        used_frames += count;
        return addr;
    }

    void pmm::free_frames(uint32_t base, uint32_t count) {
        kstd::SpinLockGuard guard(lock);
        mem_mngr.free_units(base, count);
        used_frames -= count;
    }

    uint32_t pmm::alloc_frame() {
        return alloc_frames(1);
    }

    void pmm::free_frame(uint32_t addr) {
        free_frames(addr, 1);
    }

    bool pmm::frame_used(uint32_t addr) {
        kstd::SpinLockGuard guard(lock);
        return mem_mngr.allocated(addr);
    }

    void pmm::init_region(uint32_t base, uint32_t size) {
        PageRange range = freeable_page_range(base, size);
        if (range.empty())
            return;

        mem_mngr.mark_units_free(range.base, range.frames);
    }

    void pmm::deinit_region(uint32_t base, size_t size) {
        PageRange range = covered_page_range(base, size);
        if (range.empty())
            return;

        uint32_t used_before = 0;
        for (uint32_t i = 0; i < range.frames; ++i) {
            if (mem_mngr.allocated(range.base + i * PAGE_SIZE))
                ++used_before;
        }

        mem_mngr.mark_units_used(range.base, range.frames);
        used_frames += range.frames - used_before;
    }

    uint32_t pmm::free_memory() {
        kstd::SpinLockGuard guard(lock);
        return (available_frames - used_frames) * PAGE_SIZE;
    }

    uint32_t pmm::used_memory() {
        kstd::SpinLockGuard guard(lock);
        return used_frames * PAGE_SIZE;
    }

    uint32_t pmm::total_memory() {
        kstd::SpinLockGuard guard(lock);
        return available_frames * PAGE_SIZE;
    }
}
