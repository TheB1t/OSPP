#include <sys/smp.hpp>
#include <mm/vmm.hpp>
#include <int/idt.hpp>

namespace smp {
    uint8_t Core::next_id = 0;
    kstd::vector<Core> cores;

    Core* get_core(uint8_t apic_id) {
        for (auto& core : cores) {
            if (core.apic_id == apic_id)
                return &core;
        }
        return nullptr;
    }

    Core* current_core() {
        uint32_t apic_id = apic::get_lapic_id();
        return get_core(apic_id);
    }

    void init() {
        memcpy((uint8_t*)&smp_gdt_ptr, (uint8_t*)&gdt::bsp_ptr, sizeof(gdt::Ptr));
        memcpy((uint8_t*)&smp_idt_ptr, (uint8_t*)&idt::ptr, sizeof(idt::Ptr));

        smp_pd_phy_addr = mm::vmm::kernel_dir_phys;

        for (auto& lapic : apic::get()->lapics) {
            cores.emplace_back(lapic.apic_id);
            cores.back().start();
        }
    }
}