#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <log.hpp>

namespace gdt {
    struct TSS {
        uint32_t prev_tss;   // Previous task link (unused in most OS designs)
        uint32_t esp0;       // Kernel stack pointer (CRITICAL!)
        uint32_t ss0;        // Kernel stack segment (0x10 for kernel data)
        uint32_t esp1;       // Unused (set to 0)
        uint32_t ss1;        // Unused
        uint32_t esp2;       // Unused
        uint32_t ss2;        // Unused
        uint32_t cr3;        // Page directory (for context switches)
        uint32_t eip;        // Saved instruction pointer
        uint32_t eflags;     // Saved flags register
        uint32_t eax;        // General purpose
        uint32_t ecx;
        uint32_t edx;
        uint32_t ebx;
        uint32_t esp;        // User stack pointer
        uint32_t ebp;
        uint32_t esi;
        uint32_t edi;
        uint32_t es;         // Extra segment
        uint32_t cs;         // Code segment
        uint32_t ss;         // Stack segment (user)
        uint32_t ds;         // Data segment
        uint32_t fs;         // Additional data
        uint32_t gs;         // Additional data
        uint32_t ldt;        // LDT selector (0 for most systems)
        uint16_t trap;       // Trap flag (IOPL bits)
        uint16_t iomap_base; // I/O permission bitmap offset
        // uint8_t  io_bitmap[8192]; // I/O map (optional)
        
        void initialize() {
            ss0 = 0x10;
            iomap_base = sizeof(TSS);
            trap = 0;
            cr3 = 0;
        }
        
        void set_kernel_stack(uint32_t stack_top) {
            esp0 = stack_top;
        }
    } __packed __aligned(4);

    struct Entry {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t base_middle;
        uint8_t access;
        uint8_t limit_high_flags;
        uint8_t base_high;
        
        constexpr Entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) :
            limit_low(limit & 0xFFFF),
            base_low(base & 0xFFFF),
            base_middle((base >> 16) & 0xFF),
            access(access),
            limit_high_flags(((limit >> 16) & 0x0F) | (flags << 4)),
            base_high((base >> 24) & 0xFF)
        {}
    } __packed;

    struct Ptr {
        uint16_t limit;
        uint32_t base;
    } __packed;

    enum class Type : uint8_t {
        KERNEL_CODE = 1 << 3,
        KERNEL_DATA = 2 << 3,
        USER_CODE   = 3 << 3,
        USER_DATA   = 4 << 3,
        TSS         = 5 << 3
    };

    enum Privilege : uint8_t {
        RING_0 = 0,
        RING_1 = 1,
        RING_2 = 2,
        RING_3 = 3,

        KERNEL = RING_0,
        USER   = RING_3,
    };
    
    constexpr uint8_t pack_access(bool is_code, bool rw, bool dc, bool exec, Privilege dpl, bool present) {
        return (present << 7) | (dpl << 5) | (1 << 4) | (exec << 3) | (dc << 2) | (rw << 1) | is_code;
    }

    constexpr uint8_t pack_system_access(uint8_t type, Privilege dpl, bool present) {
        return (present << 7) | (dpl << 5) | type;
    }

    constexpr uint8_t pack_flags(bool avail, bool is_32bit, bool granularity) {
        return (granularity << 3) | (is_32bit << 2) | avail;
    }

    constexpr Entry create_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
        return Entry(base, limit, access, flags);
    }

    extern TSS bsp_tss;
    extern Entry bsp_gdt[];
    extern Ptr bsp_ptr;

    template<bool is_bsp>
    void init_core(uint8_t core_id __unused) {
        if constexpr (is_bsp)
            return;

        // TODO: Implement AP bootstrapping
    }

    void init_bsp();
    void flush(const Ptr* gdtr);
}