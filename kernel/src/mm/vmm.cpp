#include <mm/vmm.hpp>
#include <sys/smp.hpp>
#include <klibcpp/atomic.hpp>

namespace mm {
    uint32_t vmm::kernel_dir_phys = 0;

    namespace {
        kstd::Atomic<uint32_t> tlb_shootdown_pending(0);
    }

    void vmm::flush_remote_tlbs() {
        smp::CoreManager* manager = smp::CoreManager::instance();
        if (!manager)
            return;

        smp::Core* current_core = smp::CoreManager::current_anchor()->core;
        if (!current_core)
            return;

        uint32_t pending = 0;
        for (uint32_t i = 0; i < manager->core_count(); ++i) {
            smp::Core& core = manager->core(i);
            if (core.apic_id == current_core->apic_id)
                continue;
            if (!core.initialized.load(kstd::MemoryOrder::Acquire))
                continue;

            ++pending;
        }

        if (!pending)
            return;

        tlb_shootdown_pending.store(pending, kstd::MemoryOrder::Release);

        for (uint32_t i = 0; i < manager->core_count(); ++i) {
            smp::Core& core = manager->core(i);
            if (core.apic_id == current_core->apic_id)
                continue;
            if (!core.initialized.load(kstd::MemoryOrder::Acquire))
                continue;

            current_core->lapic.send_ipi(core.apic_id, TLB_SHOOTDOWN_VECTOR);
        }

        while (tlb_shootdown_pending.load(kstd::MemoryOrder::Acquire) != 0)
            __pause;
    }

    void vmm::tlb_shootdown_handler(uint32_t, idt::BaseInterruptFrame*) {
        flush_current_tlb();
        tlb_shootdown_pending.fetch_sub(1, kstd::MemoryOrder::AcqRel);
    }
}
