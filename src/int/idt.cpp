#include <int/idt.hpp>
#include <driver/pic.hpp>
#include <sys/apic.hpp>
#include <log.hpp>

namespace idt {
    static Entry entries[256];
    static Ptr ptr;
    static ISRHandler isr_handlers[256];
    static IRQHandler irq_handlers[16];

    void init() {
        LOG_INFO("[idt] Initializing IDT\n");
        ptr.limit = sizeof(entries) - 1;
        ptr.base = reinterpret_cast<uint32_t>(&entries);

        for(int i = 0; i < 256; i++) {
            entries[i].set_offset(reinterpret_cast<uint32_t>(isr_table[i]));
            entries[i].selector = 0x08;
            entries[i].zero = 0;
            entries[i].type_attr = Flags::PRESENT | Flags::INTERRUPT_GATE;
        }

        flush(&ptr);
    }

    void set_entry(uint8_t vector, void (*handler)(), uint8_t flags, uint16_t selector) {
        entries[vector].set_offset(reinterpret_cast<uint32_t>(handler));
        entries[vector].selector = selector;
        entries[vector].type_attr = flags;
    }

    void register_isr(uint8_t vector, ISRHandler handler) {
        isr_handlers[vector] = handler;
    }

    void register_irq(uint8_t irq, IRQHandler handler) {
        irq_handlers[irq] = handler;
        set_entry(32 + irq, reinterpret_cast<void(*)()>(handler),
                      Flags::PRESENT | Flags::INTERRUPT_GATE);
    }

    void unregister_isr(uint8_t vector) {
        isr_handlers[vector] = nullptr;
    }

    void unregister_irq(uint8_t irq) {
        irq_handlers[irq] = nullptr;
        set_entry(32 + irq, reinterpret_cast<void(*)()>(isr_table[irq]),
                      Flags::PRESENT | Flags::INTERRUPT_GATE);
    }

    void isr_handler(uint8_t no, uint32_t err, bool has_ext, void* ctx_ptr) {
        BaseInterruptContext* ctx = reinterpret_cast<BaseInterruptContext*>(ctx_ptr);

        /*
            Temporary solution for not blowing things up.
            But if there a nested interrupt occurs, or 
            SMP is enabled, this won't work.
        */
        static char fxsave_region[512] __attribute__((aligned(16)));
        asm volatile(" fxsave %0 "::"m"(fxsave_region));

        if(no >= 32 && no < 48) { // IRQ
            uint8_t irq = no - 32;
            if(irq_handlers[irq]) {
                irq_handlers[irq](has_ext, ctx);
            } else {
                LOG_WARN("Got IRQ%d (%s), but there's no handler for it. Ignoring...\n", irq, irq_messages[irq]);
            }

            pic::EOI(irq);
        } else {
            if(isr_handlers[no]) {
                isr_handlers[no](err, has_ext, ctx);
            } else if (no < 32) {
                kstd::panic("Unhandled critical ISR: %d (%s)", no, exception_messages[no]);
            } else {
                LOG_WARN("Got ISR%d (USER), but there's no handler for it. Ignoring...\n", no);
            }
        }

        asm volatile(" fxrstor %0 "::"m"(fxsave_region));
        apic::EOI();
    }

    void flush(Ptr* idtr) {
        INLINE_ASSEMBLY(
            "movl %0, %%eax\n"
            "lidt (%%eax)\n"
            :
            : "r"(idtr)
            : "eax", "memory"
        );
    }
}