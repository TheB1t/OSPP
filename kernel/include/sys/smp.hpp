#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/trivial.hpp>
#include <klibcpp/static_slot.hpp>
#include <klibcpp/atomic.hpp>
#include <int/gdt.hpp>
#include <driver/serial.hpp>
#include <driver/pit.hpp>
#include <sys/apic.hpp>

__extern_c gdt::Ptr       smp_gdt_ptr;
__extern_c idt::Ptr       smp_idt_ptr;
__extern_c uint32_t       smp_stack_top;
__extern_c uint32_t       smp_pd_phy_addr;

static constexpr uint32_t CORE_STACK_SIZE = 4096;

class Kernel;

namespace sched {
    class Scheduler;
}

namespace smp {
    struct Core;

    struct StackAnchor {
        uint32_t magic;
        uint32_t stack_id;
        Core*    core;
        uint32_t reserved;
    };

    struct StackDescriptor {
        uint32_t     region_base;
        uint32_t     region_size;

        uint32_t     stack_begin;
        uint32_t     stack_end;

        StackAnchor* anchor;
    };

    struct Core : public NonTransferable {
        const uint8_t      id; // Logical ID
        const uint8_t      apic_id;
        const bool         is_bsp;

        kstd::Atomic<bool> initialized;
        LAPIC              lapic;
        Kernel*            kernel_;
        StackDescriptor    stack;
        alignas(16) char fxsave_region[512];

        Core(Kernel* kernel, uint32_t lapic_base, uint8_t id, uint8_t apic_id, bool is_bsp)
            : id(id), apic_id(apic_id), is_bsp(is_bsp), initialized(false), lapic(lapic_base), kernel_(kernel),
              fxsave_region{} {}

        Kernel&           kernel();
        sched::Scheduler& scheduler();
    };

    template<uint32_t N>
    struct alignas(N) CoreStack {
        static_assert((N & (N - 1)) == 0, "CoreStack size must be a power of two");
        static_assert(N > sizeof(StackAnchor), "CoreStack size too small");

        static constexpr uint32_t MAGIC = 0x434F5245;
        static constexpr uint32_t kSize = N;

        StackAnchor               anchor;
        uint8_t                   data[N - sizeof(StackAnchor)];

        void init_stack_anchor(uint32_t stack_id, Core* core) {
            anchor.magic    = MAGIC;
            anchor.stack_id = stack_id;
            anchor.core     = core;
            anchor.reserved = 0;
        }

        void copy_anchor_from(CoreStack<N>& stack) {
            anchor = stack.anchor;
        }

        constexpr uint32_t initial_sp() const {
            return base() + N;
        }

        constexpr uint32_t base() const {
            return reinterpret_cast<uint32_t>(this);
        }

        static CoreStack* from_sp(uint32_t sp) {
            return reinterpret_cast<CoreStack*>(sp & ~(N - 1));
        }

        static StackAnchor* anchor_from_sp(uint32_t sp) {
            return &from_sp(sp)->anchor;
        }
    };

    using BaseCoreStack = CoreStack<4096>;
};

__extern_c smp::BaseCoreStack stack;

namespace smp {
    class CoreManager : public NonTransferable {
        public:
            static constexpr uint16_t MAX_CORES = 255;
            inline static CoreManager* instance_ = nullptr;

            CoreManager(Kernel* kernel, uint32_t lapic_base, kstd::StaticArray<madt::ent0, 64>& lapics) {
                instance_ = this;

                LAPIC   lapic(lapic_base);

                uint8_t id          = 0;
                uint8_t bsp_apic_id = lapic.id();

                for (auto& entry : lapics) {
                    uint8_t apic_id = entry.apic_id;
                    create_core(kernel, lapic_base, id++, apic_id, apic_id == bsp_apic_id);
                }

                memcpy((uint8_t*)&smp_gdt_ptr, (uint8_t*)&gdt::bsp_ptr, sizeof(gdt::Ptr));
                memcpy((uint8_t*)&smp_idt_ptr, (uint8_t*)&idt::ptr, sizeof(idt::Ptr));

                smp_pd_phy_addr = mm::vmm::kernel_dir_phys;
            }

            void init_bsp() {
                for (uint32_t i = 0; i < core_count_; i++) {
                    Core& core_ref = core(i);

                    if (!core_ref.is_bsp)
                        continue;

                    start_core(core_ref);
                    break;
                }
            }

            void init() {
                for (uint32_t i = 0; i < core_count_; i++) {
                    Core& core_ref = core(i);

                    if (core_ref.is_bsp)
                        continue;

                    start_core(core_ref);
                }
            }

            static inline StackAnchor* current_anchor() {
                uint32_t esp;
                asm volatile ("mov %%esp, %0" : "=r"(esp));
                auto     anchor = BaseCoreStack::anchor_from_sp(esp);

                if (anchor->magic != BaseCoreStack::MAGIC)
                    kstd::panic("invalid current core stack anchor");

                return anchor;
            }

            static inline Core* current_core() {
                auto* anchor = current_anchor();
                if (!anchor->core)
                    kstd::panic("current core is null");

                return anchor->core;
            }

            uint32_t core_count() const {
                return core_count_;
            }

            static CoreManager* instance() {
                return instance_;
            }

            Core& core(uint32_t index) {
                if (index >= core_count_)
                    kstd::panic("core index out of range");

                return *core_at(index);
            }

            static void _pause() {
                __asm__ volatile (
                     "1:\n"
                     "    sti\n"
                     "    hlt\n"
                     "    jmp 1b\n"
                     :
                     :
                     : "memory"
                );
            }

        private:
            kstd::StaticSlot<Core> cores[MAX_CORES];
            uint16_t core_count_ = 0;

            template<typename... Args>
            Core& create_core(const Args&... args) {
                if (core_count_ >= MAX_CORES)
                    kstd::panic("too many cores");

                // Need to properly destruct later.
                return cores[core_count_++].construct(args...);
            }

            Core* core_at(uint32_t index) {
                return &cores[index].get();
            }

            void start_core(Core& core) {
                LOG_INFO("[smp] Initializing core %u (apic_id %u)\n", core.id, core.apic_id);
                if (core.is_bsp) {
                    StackDescriptor& desc = core.stack;

                    desc.region_base  = stack.base();
                    desc.region_size  = sizeof(stack);
                    desc.stack_begin  = desc.region_base + sizeof(StackAnchor);
                    desc.stack_end    = desc.region_base + desc.region_size;
                    desc.anchor       = current_anchor();
                    desc.anchor->core = &core;

                    core.initialized.store(true, kstd::MemoryOrder::Release);
                    return;
                }

                StackDescriptor& desc      = core.stack;
                BaseCoreStack*   new_stack = new BaseCoreStack;

                desc.region_base = (uint32_t)new_stack;
                desc.region_size = sizeof(BaseCoreStack);
                desc.stack_begin = desc.region_base + sizeof(StackAnchor);
                desc.stack_end   = desc.region_base + desc.region_size;
                desc.anchor      = &new_stack->anchor;

                new_stack->init_stack_anchor(0, &core);

                smp_stack_top = (uint32_t)desc.region_base + desc.region_size;
                kstd::atomic_thread_fence(kstd::MemoryOrder::Release);

                for (int i = 0; i < 3; i++) {
                    core.lapic.send_init_ipi(core.apic_id);
                    pit::sleep_ticks(20);
                    core.lapic.send_startup_ipi(core.apic_id, 0x8);
                    pit::sleep_ticks(20);

                    if (core.initialized.load(kstd::MemoryOrder::Acquire))
                        break;

                    LOG_WARN("[smp] Retrying core %u startup\n", core.apic_id);
                }

                if (!core.initialized.load(kstd::MemoryOrder::Acquire))
                    LOG_WARN("[smp] Failed to start core %u\n", core.apic_id);
            }
    };
};
