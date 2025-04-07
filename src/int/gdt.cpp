#include <int/gdt.hpp>

namespace gdt {
    void init_bsp() {
        LOG_INFO("[gdt] Initializing BSP's GDT. Entries %d\n", bsp_gdt_entries);
        init_core<true>(0);
        flush(&bsp_ptr);
    }

    void flush(const Ptr* gdtr) {
        INLINE_ASSEMBLY(
            "movl %0, %%eax\n"
            "lgdt (%%eax)\n"
            "jmp %1, $1f\n"
            "1:\n"
            "movw %2, %%ax\n"
            "movw %%ax, %%ds\n"
            "movw %%ax, %%es\n"
            "movw %%ax, %%fs\n"
            "movw %%ax, %%gs\n"
            "movw %%ax, %%ss\n"
            :
            : "r"(gdtr), 
              "i"(Type::KERNEL_CODE),
              "i"(Type::KERNEL_DATA)
            : "eax", "memory"
        );
    }
}