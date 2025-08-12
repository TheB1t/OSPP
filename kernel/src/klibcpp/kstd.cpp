#include <klibcpp/kstd.hpp>
#include <driver/pit.hpp>
#include <mm/vmm.hpp>

namespace kstd {
    void stack_trace(stack_frame* frame, uint32_t max_frames) {
        serial::printf("Stack trace:\n");

        for (uint32_t i = 0; frame && i < max_frames; ++i, frame = frame->ebp) {
            // TODO: Resolve symbols
            serial::printf("    [%d] 0x%08x\n", i, frame->eip);

            if (!mm::vmm::is_mapped((uint32_t)frame->ebp)) {
                serial::printf("    Invalid frame pointer: 0x%08x\n", frame->ebp);
                break;
            }
        }
    }

    void init_fpu(void) {
        uint32_t cr0, cr4;

        // Read CR0
        INLINE_ASSEMBLY("mov %%cr0, %0" : "=r"(cr0));
        // Clear EM (bit 2), set MP (bit 1)
        cr0 &= ~(1 << 2);  // EM = 0
        cr0 |=  (1 << 1);  // MP = 1
        INLINE_ASSEMBLY("mov %0, %%cr0" :: "r"(cr0));

        // Read CR4
        INLINE_ASSEMBLY("mov %%cr4, %0" : "=r"(cr4));
        // Set OSFXSR (bit 9), OSXMMEXCPT (bit 10) for SSE
        cr4 |= (1 << 9) | (1 << 10);
        INLINE_ASSEMBLY("mov %0, %%cr4" :: "r"(cr4));

        // Initialize FPU
        INLINE_ASSEMBLY("fninit");
    }

    void sleep_ticks(uint64_t target_ticks) {
        uint64_t start = pit::ticks();
        while ((pit::ticks() - start) < target_ticks)
            asm volatile("pause");
    }

    void sleep_us(uint32_t target_us) {
        uint64_t target_ticks = target_us / pit::interval();
        target_ticks = target_ticks == 0 ? 1 : target_ticks;
        sleep_ticks(target_ticks);
    }
}