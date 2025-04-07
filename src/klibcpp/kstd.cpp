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
}