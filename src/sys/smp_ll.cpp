#include <sys/smp.hpp>
#include <mm/vmm.hpp>
#include <int/idt.hpp>

__extern_c
void warm_start_32() {
    smp::Core* current = smp::current_core();
    if (current)
        current->init();
    else
        serial::printf("Failed to get current core\n");
}

__extern_c
__naked
__section(".trampoline")
void warm_start_16() {
    INLINE_ASSEMBLY(
        ".code16\n"
        "cli\n"

        "movl %%cr0, %%eax\n"
        "or $0x1, %%eax\n"
        "movl %%eax, %%cr0\n"

        "lgdt smp_gdt_ptr\n"

        "xchg %%bx, %%bx\n"
        "ljmp %[kcode], $_long_jump\n"

        ".code32\n"
        "_long_jump:\n"

        "movw %[kdata], %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"

        "lidt smp_idt_ptr\n"

        "movl smp_pd_phy_addr, %%eax\n"
        "movl %%eax, %%cr3\n"

        "movl %%cr0, %%eax\n"
        "orl $0x80000000, %%eax\n"
        "movl %%eax, %%cr0\n"

        "xor %%ebp, %%ebp\n"
        "movl smp_stack_top, %%esp\n"
        "cld\n"
        "call warm_start_32\n"     

        "cli\n"
        "hlt\n"
        "jmp .\n"

        ".global smp_gdt_ptr\n"
        ".global smp_idt_ptr\n"
        ".global smp_pd_phy_addr\n"
        ".global smp_stack_top\n"

        "smp_gdt_ptr:\n"
        ".space 6\n"

        "smp_idt_ptr:\n"
        ".space 6\n"

        "smp_pd_phy_addr:\n"
        ".space 4\n"

        "smp_stack_top:\n"
        ".space 4\n"

        ".extern warm_start_32\n"
        :
        : [kcode] "i" (gdt::Type::KERNEL_CODE),
          [kdata] "i" (gdt::Type::KERNEL_DATA)
        : "eax", "memory"
    );
}