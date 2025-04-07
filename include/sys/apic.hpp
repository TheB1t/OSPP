#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/vector.hpp>
#include <sys/acpi.hpp>
#include <io/msr.hpp>
#include <driver/pic.hpp>
#include <register.hpp>

enum class LAPICRegister : uint32_t {
    ID    = 0x20,
    VER   = 0x30,
    TPR   = 0x80,
    APR   = 0x90,
    PPR   = 0xA0,
    EOI   = 0xB0,
    RRD   = 0xC0,
    LDR   = 0xD0,
    DFR   = 0xE0,
    SIVR  = 0xF0,

    ISR0  = 0x100,
    ISR1  = 0x110,
    ISR2  = 0x120,
    ISR3  = 0x130,
    ISR4  = 0x140,
    ISR5  = 0x150,
    ISR6  = 0x160,
    ISR7  = 0x170,

    TMR0  = 0x180,
    TMR1  = 0x190,
    TMR2  = 0x1A0,
    TMR3  = 0x1B0,
    TMR4  = 0x1C0,
    TMR5  = 0x1D0,
    TMR6  = 0x1E0,
    TMR7  = 0x1F0,

    IRR0  = 0x200,
    IRR1  = 0x210,
    IRR2  = 0x220,
    IRR3  = 0x230,
    IRR4  = 0x240,
    IRR5  = 0x250,
    IRR6  = 0x260,
    IRR7  = 0x270,

    ESR   = 0x280,

    LVT_CMCI  = 0x2F0,
    ICR0      = 0x300,
    ICR1      = 0x310,
    LVT_TR    = 0x320,
    LVT_TSR   = 0x330,
    LVT_PMCR  = 0x340,
    LVT_LINT0 = 0x350,
    LVT_LINT1 = 0x360,
    LVT_ER    = 0x370,
    ICR       = 0x380,
    CCR       = 0x390,
    DCR       = 0x3E0
};

struct LAPIC {
    volatile uint32_t* base = 0;

    struct Register : public RegisterBase<Register, uint32_t> {
        LAPIC* lapic;
        const LAPICRegister reg;

        Register(LAPIC* lapic, const LAPICRegister reg) 
            : lapic(lapic), reg(reg) {}

        using RegisterBase<Register, uint32_t>::operator=;

        uint32_t read() const {
            return lapic->read(static_cast<uint32_t>(reg));
        }

        void write(uint32_t data) {
            lapic->write(static_cast<uint32_t>(reg), data);
        }
    };

    uint32_t read(uint32_t offset) {
        uint32_t result;
        INLINE_ASSEMBLY(
            "movl %1, %%eax\n"
            "andl $0xFFFF, %%eax\n"
            "addl %2, %%eax\n"
            "movl (%%eax), %%eax\n"
            : "=a"(result)
            : "r"(offset), "r"(base)
            : "memory"
        );
        return result;
    }

    void write(uint32_t offset, uint32_t value) {
        INLINE_ASSEMBLY(
            "movl %0, %%eax\n"
            "movl %1, %%ebx\n"
            "andl $0xFFFF, %%eax\n"
            "addl %2, %%eax\n"
            "movl %%ebx, (%%eax)\n"
            :
            : "r"(offset), "r"(value), "r"(base)
            : "eax", "ebx", "memory"
        );
    }

    inline Register operator[](const LAPICRegister reg) {
        return Register(this, reg);
    }

    LAPIC& operator=(uint32_t value) {
        base = (volatile uint32_t*)value;
        return *this;
    }

    operator bool() const {
        return base != 0;
    }
};

struct IOAPIC {
    uint32_t volatile* base;

    struct Register : public RegisterBase<Register, uint32_t> {
        IOAPIC* ioapic;
        uint32_t reg;

        Register(IOAPIC* ioapic, const uint32_t reg) 
            : ioapic(ioapic), reg(reg) {}

        using RegisterBase<Register, uint32_t>::operator=;

        uint32_t read() const {
            return ioapic->read(reg);
        }

        void write(uint32_t data) {
            ioapic->write(reg, data);
        }
    };

    inline Register operator[](uint32_t reg) {
        return Register(this, reg);
    }

    uint32_t read(uint32_t reg) {
        uint32_t result;
        INLINE_ASSEMBLY(
            "movl %1, %%eax\n"
            "movl %2, %%ecx\n"
            "movl %%ecx, (%%eax)\n"
            "movl 16(%%eax), %%eax\n"
            : "=a"(result)
            : "r"(base), "r"(reg)
            : "ecx", "memory"
        );
        return result;
    }

    void write(uint32_t reg, uint32_t value) {
        INLINE_ASSEMBLY(
            "movl %0, %%eax\n"
            "movl %1, %%ecx\n"
            "movl %%ecx, (%%eax)\n"
            "movl %2, %%ecx\n"
            "movl %%ecx, 16(%%eax)\n"
            :
            : "r"(base), "r"(reg), "r"(value)
            : "eax", "ecx", "memory"
        );
    }

    IOAPIC& operator=(uint32_t value) {
        base = (uint32_t volatile*)value;
        return *this;
    }

    operator bool() const {
        return base != 0;
    }
};

namespace madt {
    enum class ent_type : uint8_t {
        LAPIC = 0,
        IOAPIC = 1,
        IOAPIC_ISR_OVERRIDE = 2,
        IOAPIC_NMI_SOURCE = 3,
        LAPIC_NMI = 4,
        LAPIC_ADDR_OVERRIDE = 5,
    };

    // MADT entry header
    struct entX {
        ent_type    type;
        uint8_t     length;
    } __packed;

    // MADT Core entry
    struct ent0 : public entX {
        uint8_t     acpi_processor_id;
        uint8_t     apic_id;
        uint32_t    cpu_flags;
    } __packed;

    // MADT IOAPIC entry
    struct ent1 : public entX {
        uint8_t     ioapic_id;
        uint8_t     reserved;
        IOAPIC      ioapic;
        uint32_t    gsi_base;
    } __packed;

    // MADT IRQ override entry
    struct ent2 : public entX {
        uint8_t     bus_src;
        uint8_t     irq_src;
        uint32_t    gsi;
        uint16_t    flags;
    } __packed;

    // MADT NMI source entry
    struct ent3 : public entX {
        uint8_t     nmi_src;
        uint8_t     reserved;
        uint32_t    flags;
        uint32_t    gsi;
    } __packed;

    // MADT LINT entry
    struct ent4 : public entX {
        uint8_t     acpi_processor_id;
        uint16_t    flags;
        uint8_t     lint;
    } __packed;

    // MADT LAPIC override entry
    struct ent5 : public entX {
        uint16_t    reserved;
        uint64_t    local_apic_override;
    } __packed;
};

class apic {
    public:
        struct MADT : public acpi::SDTHeader {
            uint32_t    local_apic_addr;
            uint32_t    apic_flags;
        } __packed;

        kstd::vector<madt::ent0> lapics;
        kstd::vector<madt::ent1> ioapics;
        kstd::vector<madt::ent2> irq_overrides;
        kstd::vector<madt::ent3> nmi_sources;
        kstd::vector<madt::ent4> lint_sources;

        static LAPIC lapic_base;

        static apic* get() {
            static apic instance;
            return &instance;
        }

        apic(const apic&) = delete;
        apic& operator=(const apic&) = delete;

        static void EOI() {
            lapic_base[LAPICRegister::EOI] = (uint32_t)0;
        }

        static void enable() {
            write_msr(BASE_MSR, (read_msr(BASE_MSR) | BASE_MSR_ENABLE) & ~(1 << 10));
            lapic_base[LAPICRegister::SIVR] |= 0x1FF;
        }

        void configure() {
            bool legacy_mapped_irqs[16] = {};
            uint32_t lapic_id = lapic_base[LAPICRegister::ID].read() >> 24;

            for (auto& ioapic : ioapics) {
                uint32_t gsi_max = get_gsi_max(&ioapic) + ioapic.gsi_base;

                LOG_INFO("[apic][IOAPIC#%u] Masked GSI's %d-%d\n", ioapic.ioapic_id, ioapic.gsi_base, gsi_max);
                for (uint32_t gsi = ioapic.gsi_base; gsi < gsi_max; gsi++)
                    mask_gsi(&ioapic, gsi);

                LOG_INFO("[apic][IOAPIC#%u] Mapped IRQ's:\n", ioapic.ioapic_id);
                for (auto& irq : irq_overrides) {
                    if (irq.gsi >= ioapic.gsi_base && irq.gsi < gsi_max) {
                        if (!legacy_mapped_irqs[irq.gsi]) {
                            redirect_gsi(&ioapic, irq.irq_src, irq.gsi, irq.flags, lapic_id);
                            if (irq.gsi < 16)
                                legacy_mapped_irqs[irq.gsi] = 1;

                            LOG_INFO("    %3u GSI -> %3u IRQ\n", irq.gsi, irq.irq_src);
                        }
                    }
                }
            }

            LOG_INFO("[apic] Remapped IRQ's:\n");
            for (uint8_t i = 0; i < 16; i++) {
                if (!legacy_mapped_irqs[i]) {
                    madt::ent1* ioapic = find_valid_ioapic(i);
                    redirect_gsi(ioapic, i, (uint32_t)i, 0, lapic_id);

                    LOG_INFO("    %3u GSI -> %3u IRQ\n", i, i);
                }
            }
        }

    private:
        static constexpr uint32_t BASE_MSR = 0x1B;
        static constexpr uint32_t BASE_MSR_ENABLE = 0x800;
        static constexpr uint64_t REDIRECT_TABLE_BAD_READ = 0xFFFFFFFFFFFFFFFF;

        apic() {
            auto madt = acpi::find_sdth<MADT>("APIC");
            if (!madt) {
                LOG_INFO("[apic] Can't find MADT\n");
                return;
            }

            uint32_t lapic = madt->local_apic_addr;
            LOG_INFO("[apic] MADT found at 0x%08x\n", madt);
            mm::vmm::map_page((uint32_t)madt, (uint32_t)madt, mm::Flags::Present | mm::Flags::Writable);

            madt::entX* ent = reinterpret_cast<madt::entX*>(reinterpret_cast<uint8_t*>(madt) + sizeof(MADT));;
            int32_t size_bytes = madt->length - sizeof(MADT);

            LOG_INFO("[apic] MADT entries:\n");
            while (size_bytes > 0) {
                uint8_t current_length = ent->length;

                switch (ent->type) {
                    case madt::ent_type::LAPIC: {
                        auto ent0 = (madt::ent0*)ent;
                        LOG_INFO("    LAPIC: ACPI ID %2d, APIC ID %2d, flags 0x%08x\n", ent0->acpi_processor_id, ent0->apic_id, ent0->cpu_flags);
                        lapics.push_back(*ent0);
                        break;
                    }
                    case madt::ent_type::IOAPIC: {
                        auto ent1 = (madt::ent1*)ent;
                        LOG_INFO("    IOAPIC: ID %d, addr 0x%08x, GSI base 0x%08x\n", ent1->ioapic_id, ent1->ioapic.base, ent1->gsi_base);
                        ioapics.push_back(*ent1);

                        mm::vmm::map_page((uint32_t)ent1->ioapic.base, (uint32_t)ent1->ioapic.base, mm::Flags::Present | mm::Flags::Writable);
                        break;
                    }
                    case madt::ent_type::IOAPIC_ISR_OVERRIDE: {
                        auto ent2 = (madt::ent2*)ent;
                        LOG_INFO("    IRQ override: bus %2d, IRQ %2d, GSI %2d, flags 0x%04x\n", ent2->bus_src, ent2->irq_src, ent2->gsi, ent2->flags);
                        irq_overrides.push_back(*ent2);
                        break;
                    }
                    case madt::ent_type::IOAPIC_NMI_SOURCE: {
                        auto ent3 = (madt::ent3*)ent;
                        LOG_INFO("    NMI source: src %d, flags 0x%08x, GSI 0x%08x\n", ent3->nmi_src, ent3->flags, ent3->gsi);
                        nmi_sources.push_back(*ent3);
                        break;
                    }
                    case madt::ent_type::LAPIC_NMI: {
                        auto ent4 = (madt::ent4*)ent;
                        LOG_INFO("    LINT: ACPI ID %d, flags 0x%04x, LINT %d\n", ent4->acpi_processor_id, ent4->flags, ent4->lint);
                        lint_sources.push_back(*ent4);
                        break;
                    }
                    case madt::ent_type::LAPIC_ADDR_OVERRIDE: {
                        auto ent5 = (madt::ent5*)ent;
                        LOG_INFO("    LAPIC override at 0x%08x\n", ent5->local_apic_override);
                        lapic = ent5->local_apic_override;
                        break;
                    }
                    default:
                        LOG_INFO("    Unknown entry type %d\n", ent->type);
                }

                ent = reinterpret_cast<madt::entX*>(reinterpret_cast<uint8_t*>(ent) + current_length);
                size_bytes -= current_length;
            }

            mm::vmm::unmap_page((uint32_t)madt);

            LOG_INFO("[apic] LAPIC base: 0x%08x\n", lapic);
            mm::vmm::map_page(lapic, lapic, mm::Flags::Present | mm::Flags::Writable);

            lapic_base = lapic;
        }

        uint8_t get_gsi_max(madt::ent1* ioapic) {
            uint32_t data = ioapic->ioapic[1];
            uint8_t gsi_max = (data >> 16) & 0xFF;
            return gsi_max & ~(1 << 7);
        }

        madt::ent1* find_valid_ioapic(uint32_t gsi) {
            madt::ent1* valid_ioapic = nullptr;

            for (auto& ioapic : ioapics) {
                uint32_t gsi_max = get_gsi_max(&ioapic) + ioapic.gsi_base;

                if (ioapic.gsi_base <= gsi && gsi_max >= gsi) {
                    valid_ioapic = &ioapic;
                    break;
                }
            }

            return valid_ioapic;
        }

        bool write_redirection_table(madt::ent1* ioapic, uint32_t gsi, uint64_t data) {
            if (!ioapic)
                return false;

            uint32_t reg = ((gsi - ioapic->gsi_base) * 2) + 16;
            ioapic->ioapic[reg] = data & 0xFFFFFFFF;
            ioapic->ioapic[reg + 1] = data >> 32;
            return true;
        }

        uint64_t read_redirection_table(madt::ent1* ioapic, uint32_t gsi) {
            if (!ioapic)
                return REDIRECT_TABLE_BAD_READ;

            uint32_t reg = ((gsi - ioapic->gsi_base) * 2) + 16;
            uint64_t data = ioapic->ioapic[reg]; 
            data |= ((uint64_t)ioapic->ioapic[reg + 1] << 32);
            return data;
        }

        bool mask_gsi(madt::ent1* ioapic, uint32_t gsi) {
            uint64_t current_data = read_redirection_table(ioapic, gsi);
            if (current_data == REDIRECT_TABLE_BAD_READ)
                return false;

            return write_redirection_table(ioapic, gsi, current_data | (1 << 16));
        }

        bool redirect_gsi(madt::ent1* ioapic, uint8_t irq, uint32_t gsi, uint16_t flags, uint8_t apic) {
            uint64_t redirect = (uint64_t)irq + 32;

            if (flags & 1 << 1)
                redirect |= 1 << 13;

            if (flags & 1 << 3)
                redirect |= 1 << 15;

            redirect |= ((uint64_t)apic) << 56;
            return write_redirection_table(ioapic, gsi, redirect);
        }
};