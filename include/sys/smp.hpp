#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/vector.hpp>
#include <int/gdt.hpp>
#include <driver/serial.hpp>
#include <sys/apic.hpp>

__extern_c uint8_t stack[4096];             // BSP stack

__extern_c gdt::Ptr smp_gdt_ptr;
__extern_c idt::Ptr smp_idt_ptr;
__extern_c uint32_t smp_stack_top;
__extern_c uint32_t smp_pd_phy_addr;

namespace smp {
    class Core {
        public:
            const uint8_t id;   // Logical ID
            const uint8_t apic_id;

            Core(uint8_t apic_id) 
                : id(next_id++), apic_id(apic_id), stack_size(0), stack_base(0) {}

            void start() {
                uint32_t bsp_apic_id = apic::get_lapic_id();

                LOG_INFO("Initializing core %d (apic_id %d)\n", id, apic_id);
                if (apic_id == bsp_apic_id) {
                    stack_size = sizeof(stack);
                    stack_base = stack;

                    LOG_INFO("Core %d is the BSP, skipping...\n", id);
                    return;
                }

                stack_size = 0x1000;
                stack_base = new uint8_t[stack_size];

                smp_stack_top = (uint32_t)stack_base + stack_size;

                for (int i = 0; i < 3; i++) {
                    apic::send_init_ipi(apic_id);
                    kstd::sleep_ticks(20);
                    apic::send_startup_ipi(apic_id, 0x8);
                    kstd::sleep_ticks(20);

                    if (initialized)
                        break;
                }

                if (!initialized)
                    LOG_WARN("Failed to warm start core %d\n", apic_id);
            }

            void init() {
                /*
                    GDT, IDT and Paging already setted up at this point
                    They equal to BSP's GDT, IDT and Paging
                */
                kstd::init_fpu();
            
                initialized = true;
                LOG_INFO("Core %d is ready\n", id);
                _pause(); // Go to sleep
            }

            static void _pause() {
                INLINE_ASSEMBLY(
                    "1:\n"
                    "    pause\n"
                    "    jmp 1b\n"
                );
            }

        private:
            bool initialized = false;

            uint32_t stack_size;
            uint8_t* stack_base;

            static uint8_t next_id;
    };

    extern kstd::vector<Core> cores;

    Core* get_core(uint8_t apic_id);
    Core* current_core();
    void init();
};