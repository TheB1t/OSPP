#include <int/gdt.hpp>

namespace gdt {
    TSS bsp_tss{};

    Entry bsp_gdt[] = {
        Entry(0, 0, 0, 0), // Null
        Entry(0, 0xFFFFF, pack_access(1, 1, 0, 1, Privilege::KERNEL, 1), pack_flags(0, 1, 1)), // Kernel CS
        Entry(0, 0xFFFFF, pack_access(1, 1, 0, 0, Privilege::KERNEL, 1), pack_flags(0, 1, 1)), // Kernel DS
        Entry(0, 0xFFFFF, pack_access(1, 1, 0, 1, Privilege::USER, 1), pack_flags(0, 1, 1)), // User CS
        Entry(0, 0xFFFFF, pack_access(1, 1, 0, 0, Privilege::USER, 1), pack_flags(0, 1, 1)), // User DS
        Entry(reinterpret_cast<uint32_t>(&bsp_tss), sizeof(TSS)-1, pack_system_access(0x9, Privilege::USER, 1), pack_flags(0, 1, 1)), // TSS
    };

    Ptr bsp_ptr = {
        .limit = sizeof(bsp_gdt) - 1,
        .base = reinterpret_cast<uint32_t>(bsp_gdt)
    };

    void init_bsp() {
        LOG_INFO("[gdt] Initializing BSP's GDT\n");
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