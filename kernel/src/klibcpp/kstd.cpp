#include <klibcpp/kstd.hpp>
#include <driver/pit.hpp>
#include <mm/vmm.hpp>

namespace kstd {
    namespace ByteConverter {
        float convert(float value, Unit from, Unit to) {
            float bytes = value * unitFactor(from);
            return bytes / unitFactor(to);
        }

        Unit bestUnit(float value) {
            Unit units[] = { Unit::B, Unit::KB, Unit::MB, Unit::GB, Unit::TB };
            int  i       = 0;
            while (i < 4 && value >= 1024.0) {
                value /= 1024.0;
                ++i;
            }
            return units[i];
        }
    }

    void _panic(const char* msg) {
        serial::printf("\n\nKernel panic:\n%s\n", msg);

        stack_frame* stk;
        __asm__ volatile ("movl %%ebp, %0" : "=r"(stk));

        stack_trace(stk, 10);
        __unreachable();
    }

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

    void init_fpu() {
        uint32_t cr0, cr4;

        // Read CR0
        __asm__ volatile ("mov %%cr0, %0" : "=r" (cr0));
        // Clear EM (bit 2), set MP (bit 1)
        cr0 &= ~(1 << 2);  // EM = 0
        cr0 |=  (1 << 1);  // MP = 1
        __asm__ volatile ("mov %0, %%cr0" :: "r" (cr0));

        // Read CR4
        __asm__ volatile ("mov %%cr4, %0" : "=r" (cr4));
        // Set OSFXSR (bit 9), OSXMMEXCPT (bit 10) for SSE
        cr4 |= (1 << 9) | (1 << 10);
        __asm__ volatile ("mov %0, %%cr4" :: "r" (cr4));

        // Initialize FPU
        __asm__ volatile ("fninit");
    }
}