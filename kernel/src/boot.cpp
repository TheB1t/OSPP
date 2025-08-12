#include <multiboot.hpp>
#include <klibcpp/cstdint.hpp>

__extern_c void kernel_early_main(multiboot_info_t* mboot, uint32_t magic);

struct _multiboot_header_ {
    uint32_t magic;          // Must be 0x1BADB002
    uint32_t flags;          // Feature flags
    uint32_t checksum;       // magic + flags + checksum = 0
} __packed __aligned(4);

constexpr uint32_t calculate_checksum(const _multiboot_header_& header) {
    return -(header.magic + header.flags);
}

__used
__section(".multiboot")
const _multiboot_header_ mboot_header = {
    .magic = MULTIBOOT_HEADER_MAGIC,
    .flags = MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO,
    .checksum = calculate_checksum(mboot_header),
};

__used
__aligned(16)
__section(".bss")
uint8_t stack[4096] = {0};

__extern_c
__naked 
void cold_start() {
    INLINE_ASSEMBLY(
        "xor %%ebp, %%ebp\n"            // Clear ebp for stack trace
        "mov %[stack_top], %%esp\n"     // Set stack pointer

        "push %%eax\n"                  // Push magic
        "push %%ebx\n"                  // Push multiboot info

        "mov %[kernel_entry], %%eax\n"  
        "call *%%eax\n"                 // Call kernel entry point

        ".Lhalt:\n"
        "hlt\n"
        "jmp .Lhalt\n"
        : 
        : [stack_top] "i" (stack + sizeof(stack)),
          [kernel_entry] "i" (kernel_early_main)
        : "memory"
    );
}