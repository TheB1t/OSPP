#pragma once

#include <klibc/cstdint.hpp>
#include <io/ports.hpp>

__extern_asm void idt_flush(uint32_t);

namespace idt {
    struct Entry {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t zero;
        uint8_t type_attr;
        uint16_t offset_high;
        
        void set_offset(uint32_t offset) {
            offset_low = offset & 0xFFFF;
            offset_high = (offset >> 16) & 0xFFFF;
        }
    } __packed;

    struct Ptr {
        uint16_t limit;
        uint32_t base;
    } __packed;

    enum Flags {
        PRESENT = 0x80,
        RING0 = 0x00,
        RING3 = 0x60,
        INTERRUPT_GATE = 0x0E,
        TRAP_GATE = 0x0F
    };

    struct Context {
        uint32_t gs, fs, es, ds;
        uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
        uint32_t int_no, err_code;
        uint32_t eip, cs, eflags, useresp, ss;
    };

    extern Entry entries[256];
    extern Ptr ptr;

    void init();
    void set_entry(uint8_t vector, void (*handler)(), uint8_t flags, uint16_t selector = 0x08);
}

extern "C" {
    typedef void (*ISRHandler)(idt::Context* context);
    typedef void (*IRQHandler)(idt::Context* context);
    
    void register_isr(uint8_t vector, ISRHandler handler);
    void register_irq(uint8_t irq, IRQHandler handler);
}