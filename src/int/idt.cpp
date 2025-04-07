#include <int/idt.hpp>
#include <kutils.hpp>
#include <driver/serial.hpp>

__extern_asm void* isr_table[];

namespace idt {
    Entry entries[256];
    Ptr ptr;

    static constexpr const char* exception_messages[] = {
        "Division by zero",
        "Debug",
        "Non-maskable interrupt",
        "Breakpoint",
        "Detected overflow",
        "Out of bounds",
        "Invalid opcode",
        "No coprocessor",
        "Double fault",
        "Coprocessor segment overrun",
        "Bad TSS",
        "Segment not present",
        "Stack fault",
        "General protection fault",
        "Page fault",
        "Unknown interrupt",
        "Coprocessor fault",
        "Alignment check",
        "Machine check",
    };

    static ISRHandler isr_handlers[256];
    static IRQHandler irq_handlers[16];

    void init() {
        serial::printf("[idt] Initializing IDT\n");
        ptr.limit = sizeof(entries) - 1;
        ptr.base = reinterpret_cast<uint32_t>(&entries);
        
        for(int i = 0; i < 256; i++) {
            entries[i].set_offset(reinterpret_cast<uint32_t>(isr_table[i]));
            entries[i].selector = 0x08;
            entries[i].zero = 0;
            entries[i].type_attr = Flags::PRESENT | Flags::INTERRUPT_GATE;
        }

        idt_flush(reinterpret_cast<uint32_t>(&ptr));
    }

    void set_entry(uint8_t vector, void (*handler)(), uint8_t flags, uint16_t selector) {
        entries[vector].set_offset(reinterpret_cast<uint32_t>(handler));
        entries[vector].selector = selector;
        entries[vector].type_attr = flags;
    }
}

extern "C" {
    void register_isr(uint8_t vector, ISRHandler handler) {
        idt::isr_handlers[vector] = handler;
    }
    
    void register_irq(uint8_t irq, IRQHandler handler) {
        idt::irq_handlers[irq] = handler;
        idt::set_entry(32 + irq, reinterpret_cast<void(*)()>(handler),
                      idt::Flags::PRESENT | idt::Flags::INTERRUPT_GATE);
    }

    void unregister_isr(uint8_t vector) {
        idt::isr_handlers[vector] = nullptr;
    }

    void unregister_irq(uint8_t irq) {
        idt::irq_handlers[irq] = nullptr;
        idt::set_entry(32 + irq, reinterpret_cast<void(*)()>(isr_table[irq]),
                      idt::Flags::PRESENT | idt::Flags::INTERRUPT_GATE);
    }

    void isr_handler(idt::Context* context) {
        uint32_t int_no = context->int_no;

        if(int_no >= 32 && int_no < 48) { // IRQ
            uint8_t irq = int_no - 32;
            if(idt::irq_handlers[irq]) {
                idt::irq_handlers[irq](context);
            } else {
                serial::printf("Got IRQ%d, but there's no handler for it. Ignoring...\n", irq);
            }

            if(int_no >= 40) 
                ports::outb(0xA0, 0x20); // Send EOI to slave

            ports::outb(0x20, 0x20); // Send EOI to master
        } else {
            if(idt::isr_handlers[int_no]) {
                idt::isr_handlers[int_no](context);
            } else if (int_no < 32) {
                kutils::panic("Unhandled critical ISR: %d (%s)", int_no, idt::exception_messages[int_no]);
            }
        }
    }
}