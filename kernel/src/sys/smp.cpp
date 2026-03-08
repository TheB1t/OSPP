#include <sys/smp.hpp>
#include <kernel.hpp>
#include <mm/vmm.hpp>
#include <int/idt.hpp>

namespace smp {
    Kernel& Core::kernel() {
        if (!kernel_)
            kstd::panic("core has no kernel");

        return *kernel_;
    }

    sched::Scheduler& Core::scheduler() {
        auto* scheduler = &kernel()._sched.get();
        if (!scheduler)
            kstd::panic("core scheduler is not constructed");

        return *scheduler;
    }
}
