#include <klibcpp/kstd.hpp>
#include <module.hpp>

namespace kstd {
    void stack_trace(stack_frame* frame, uint32_t max_frames) {
        serial::printf("Stack trace:\n");

        auto debug = k::KernelModuleRegistry::get()->debug_module();
        if (!debug)
            serial::printf("Printing stack trace without debug symbols\n");

        for (uint32_t i = 0; frame && i < max_frames; ++i, frame = frame->ebp) {
            if (!debug) {
                serial::printf("    [%d] 0x%08x\n", i, frame->eip);
            } else {
                auto sym = debug->lookup_symbol(frame->eip);
                if (sym)
                    serial::printf("    [%d] 0x%08x : %s\n", i, frame->eip, sym);
                else
                    serial::printf("    [%d] 0x%08x\n", i, frame->eip);
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
}