#include <int/idt.hpp>
#include <driver/pic.hpp>
#include <sys/smp.hpp>
#include <log.hpp>

namespace idt {
    static ISRHandler isr_handlers[256];
    static IRQHandler irq_handlers[16];

    static void set_entry(uint8_t vector, void (*handler)(), uint8_t flags, uint16_t selector = 0x08) {
        entries[vector].set_offset(reinterpret_cast<uint32_t>(handler));
        entries[vector].selector  = selector;
        entries[vector].type_attr = flags;
    }

    Entry entries[256];
    Ptr   ptr{
        .limit = sizeof(entries) - 1,
        .base  = reinterpret_cast<uint32_t>(&entries),
    };

    void init() {
        LOG_INFO("[idt] Initializing IDT\n");

        for(int i = 0; i < 256; i++) {
            entries[i].zero = 0;
            set_entry(i, reinterpret_cast<void (*)()>(isr_table[i].entry), Flags::PRESENT | Flags::INTERRUPT_GATE);
        }

        flush(&ptr);
    }

    void register_isr(uint8_t vector, ISRHandler handler) {
        isr_handlers[vector] = handler;
    }

    void register_irq(uint8_t irq, IRQHandler handler) {
        irq_handlers[irq] = handler;
    }

    void unregister_isr(uint8_t vector) {
        isr_handlers[vector] = nullptr;
    }

    void unregister_irq(uint8_t irq) {
        irq_handlers[irq] = nullptr;
    }

    void isr_handler(uint8_t no, uint32_t err, void* ctx_ptr) {
        BaseInterruptFrame* ctx = reinterpret_cast<BaseInterruptFrame*>(ctx_ptr);
        // TODO: save FPU state per-core or per-context; this static buffer is not SMP-safe.
        static char fxsave_region[512] __attribute__((aligned(16)));
        __asm__ volatile (" fxsave %0 " ::"m" (fxsave_region));

        if(no >= 32 && no < 48) { // IRQ
            uint8_t irq = no - 32;
            if(irq_handlers[irq]) {
                irq_handlers[irq](ctx);
            } else {
                LOG_WARN("[idt] Unhandled IRQ%u (%s), ignoring\n", irq, irq_messages[irq]);
            }

            pic::EOI(irq);
        } else {
            if(isr_handlers[no]) {
                isr_handlers[no](err, ctx);
            } else if (no < 32) {
                kstd::panic("Unhandled critical ISR: %d (%s)", no, exception_messages[no]);
            } else {
                LOG_WARN("[idt] Unhandled ISR%u (user), ignoring\n", no);
            }
        }

        __asm__ volatile (" fxrstor %0 " ::"m" (fxsave_region));
        if (smp::CoreManager::current_anchor()->core)
            smp::CoreManager::current_anchor()->core->lapic.EOI();
    }

    void flush(const Ptr* idtr) {
        __asm__ volatile (
             "movl %0, %%eax\n"
             "lidt (%%eax)\n"
             :
             : "r" (idtr)
             : "eax", "memory"
        );
    }
}
