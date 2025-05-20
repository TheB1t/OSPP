#pragma once

#include <klibcpp/kstd.hpp>
#include <klibcpp/array.hpp>
#include <klibcpp/cstdint.hpp>
#include <io/ports.hpp>

namespace idt {
    struct BaseInterruptContext {
        uint32_t eip, cs, eflags;
        uint32_t user_esp, user_ss; // SP and SS pushed only if CPL!=RPL
    } __packed;

    struct InterruptContext {
        uint32_t ds, es, fs, gs;
        uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
        uint32_t int_no, err_code;

        BaseInterruptContext base;

        /*
            !!! WARNING !!!

            This method must be used exclusively with the original pointer to
            BaseInterruptContext provided by IRQ/ISR handlers. These handlers
            are responsible for constructing an InterruptContext on top of
            the BaseInterruptContext.

            Invoking this method in any other context may lead to undefined
            behavior.

            Note: GNU interrupt handlers instantiate only BaseInterruptContext.

            Use with extreme caution.
        */
        static InterruptContext* from_base(BaseInterruptContext* base) {
            // Calculate the offset of `base` inside `InterruptContext`
            constexpr size_t offset = __builtin_offsetof(InterruptContext, base);
            // Subtract the offset to get the enclosing `InterruptContext*`
            return reinterpret_cast<InterruptContext*>(reinterpret_cast<char*>(base) - offset);
        }

    } __packed;

    void isr_handler(uint8_t no, uint32_t err, bool has_ext, void* ctx_ptr);
}

/*
    ISR wrappers
*/
#pragma GCC push_options
// Disable SSE/MMX/FPU
#pragma GCC target ("general-regs-only")
    template<int N>
    __interrupt void isr_wrapper_no_err(void* ctx) {
        idt::isr_handler(N, 0, false,ctx);
    }

    template<int N>
    __interrupt void isr_wrapper_err(void* ctx, uint32_t err) {
        idt::isr_handler(N, err, false, ctx);
    }

    template<int N>
    __naked void context_switch() {
        asm volatile (
            "push $0\n"                     // push err_code
            "push %[int_no]\n"              // push int_no

            "pusha\n"

            "xor %%eax, %%eax\n"

            "mov %%gs, %%ax\n"
            "push %%eax\n"

            "mov %%fs, %%ax\n"
            "push %%eax\n"

            "mov %%es, %%ax\n"
            "push %%eax\n"

            "mov %%ds, %%ax\n"
            "push %%eax\n"

            "mov $0x10, %%ax\n"
            "mov %%ax, %%ds\n"
            "mov %%ax, %%es\n"
            "mov %%ax, %%fs\n"
            "mov %%ax, %%gs\n"

            "mov %%esp, %%eax\n"
            "add %[base_offset], %%eax\n"   // eax = &BaseInterruptContext

            "push %%eax\n"                  // ctx = &BaseInterruptContext
            "push $1\n"                     // has_ext = true
            "push $0\n"                     // err = 0
            "push %[int_no]\n"              // int_no = N

            "mov %[handler], %%eax\n"       // eax = &idt::isr_handler
            "call *%%eax\n"                 // call idt::isr_handler(N, 0, true, &BaseInterruptContext)

            "add $16, %%esp\n"              // pop args, 4 * 4 = 16

            "xor %%eax, %%eax\n"

            "pop %%eax\n"
            "mov %%ax, %%ds\n"

            "pop %%eax\n"
            "mov %%ax, %%es\n"

            "pop %%eax\n"
            "mov %%ax, %%fs\n"

            "pop %%eax\n"
            "mov %%ax, %%gs\n"

            "popa\n"

            "add $8, %%esp\n"               // pop err_code, int_no

            "iret\n"
            :
            : [int_no] "i" (N),
              [base_offset] "i" (__builtin_offsetof(idt::InterruptContext, base)),
              [handler] "i" (&idt::isr_handler)
            : "memory", "eax"
        );
    }
#pragma GCC pop_options

namespace idt {
    template <int N>
    constexpr uint32_t get_isr_wrapper() {
        if constexpr (N == 32 || N == 14)
            return (uint32_t)&context_switch<N>;

        if constexpr (N == 8 || N == 10 || N == 11 || N == 12 || N == 13 || N == 14 || N == 17 || N == 21)
            return (uint32_t)&isr_wrapper_err<N>;

        return (uint32_t)&isr_wrapper_no_err<N>;
    }

    template <size_t... IntNums>
    constexpr auto make_isr_table_impl(kstd::index_sequence<IntNums...>) {
        return kstd::array<uint32_t, sizeof...(IntNums)>{{ 
            get_isr_wrapper<IntNums>()... 
        }};
    }

    const kstd::array<uint32_t, 256> isr_table = make_isr_table_impl(kstd::make_index_sequence<256>{});

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

    extern Entry entries[256];
    extern Ptr ptr;

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

    static constexpr const char* irq_messages[] = {
        "PIT",
        "Keyboard",
        "Cascade",
        "COM2/COM4",
        "COM1/COM3",
        "LPT2",
        "Floppy",
        "LPT1",
        "RTC",
        "Not used",
        "Not used",
        "Not used",
        "Mouse",
        "Math coprocessor",
        "ATA1",
        "ATA2",
    };

    using ISRHandler = void(*)(uint32_t, bool, BaseInterruptContext*);
    using IRQHandler = void(*)(bool, BaseInterruptContext*);

    void init();
    void set_entry(uint8_t vector, void (*handler)(), uint8_t flags, uint16_t selector = 0x08);
    void register_isr(uint8_t vector, ISRHandler handler);
    void register_irq(uint8_t irq, IRQHandler handler);
    void unregister_isr(uint8_t vector);
    void unregister_irq(uint8_t irq);

    void flush(const Ptr* idtr);
}