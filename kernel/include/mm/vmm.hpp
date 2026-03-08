#pragma once

#include <klibcpp/kstd.hpp>
#include <klibcpp/spinlock.hpp>
#include <klibcpp/cstdlib.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <int/idt.hpp>
#include <mm/layout.hpp>
#include <mm/pmm.hpp>

namespace mm {
    static constexpr uint32_t PDE_BASE  = layout::virt::RECURSIVE_PD_BASE;
    static constexpr uint32_t PT_BASE   = layout::virt::RECURSIVE_PT_BASE;
    static constexpr uint32_t PAGE_MASK = 0xFFFFF000;

    enum Flags : uint32_t {
        All           = 0,
        Present       = 1 << 0,
        Writable      = 1 << 1,
        User          = 1 << 2,
        WriteThrough  = 1 << 3,
        CacheDisabled = 1 << 4,
        Accessed      = 1 << 5,
        Dirty         = 1 << 6,
        HugePage      = 1 << 7,
    };

    struct Entry {
        uint32_t                  value        = 0;

        static constexpr uint32_t ADDRESS_MASK = 0xFFFFF000;
        static constexpr uint32_t FLAGS_MASK   = 0x00000FFF;

        uint32_t address() const {
            return value & ADDRESS_MASK;
        }

        void set_address(uint32_t physical_addr) {
            value = (value & FLAGS_MASK) | (physical_addr & ADDRESS_MASK);
        }

        void invalidate() {
            value = 0;
        }

        bool has_flag(uint32_t flag) const {
            return (value & FLAGS_MASK & flag) == flag;
        }

        void update_flags(uint32_t flags) {
            value |= (flags & FLAGS_MASK);
        }
    };

    static_assert(sizeof(Entry) == 4, "Entry size must be 4 bytes");

    struct AlignedResult {
        uint32_t aligned;
        uint32_t offset;
    };

    static inline AlignedResult align_address(uint32_t address) {
        return {
            address& PAGE_MASK,
            address & (PAGE_SIZE - 1)
        };
    }

    static inline uint32_t pde_index(uint32_t virt_addr) {
        return (virt_addr >> 22) & 0x3FF;
    }

    static inline uint32_t pte_index(uint32_t virt_addr) {
        return (virt_addr >> 12) & 0x3FF;
    }

    static inline Entry& pde_entry(uint32_t virt_addr) {
        return *reinterpret_cast<Entry*>(PDE_BASE + pde_index(virt_addr) * sizeof(Entry));
    }

    static inline Entry& pte_entry(uint32_t virt_addr) {
        const uint32_t pt_virt = PT_BASE + (pde_index(virt_addr) << 12);
        return *reinterpret_cast<Entry*>(pt_virt + pte_index(virt_addr) * sizeof(Entry));
    }

    static inline uint32_t pt_virtual_base(uint32_t virt_addr) {
        return PT_BASE + (pde_index(virt_addr) << 12);
    }

    class vmm {
        public:
            static constexpr uint8_t TLB_SHOOTDOWN_VECTOR = 49;
            static uint32_t kernel_dir_phys;

            static void init() {
                uint32_t pd_phys = pmm::alloc_frame();
                uint32_t pt_phys = pmm::alloc_frame();

                if (!pd_phys || !pt_phys)
                    kstd::panic("vmm::init: out of physical memory");

                // Assumes low physical memory is identity-mapped at bootstrap.
                Entry* pd = reinterpret_cast<Entry*>(pd_phys);
                Entry* pt = reinterpret_cast<Entry*>(pt_phys);

                memset(reinterpret_cast<uint8_t*>(pd), 0, PAGE_SIZE);
                memset(reinterpret_cast<uint8_t*>(pt), 0, PAGE_SIZE);

                for (uint32_t i = 0; i < (layout::virt::IDENTITY_WINDOW_SIZE / PAGE_SIZE); ++i) {
                    pt[i].set_address(i * PAGE_SIZE);
                    pt[i].update_flags(Present | Writable);
                }

                pd[0].set_address(pt_phys);
                pd[0].update_flags(Present | Writable);

                pd[1023].set_address(pd_phys);
                pd[1023].update_flags(Present | Writable);

                idt::register_isr(14, page_fault);
                idt::register_isr(TLB_SHOOTDOWN_VECTOR, tlb_shootdown_handler);

                kernel_dir_phys = pd_phys;
                load_directory(kernel_dir_phys);
                enable_paging();
            }

            static void map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
                kstd::SpinLockGuard guard(lock_);
                map_page_locked(virt_addr, phys_addr, flags);
                flush_remote_tlbs();
            }

            static void map_pages(uint32_t virt_addr, uint32_t phys_addr, uint32_t pages, uint32_t flags) {
                kstd::SpinLockGuard guard(lock_);

                virt_addr = align_address(virt_addr).aligned;
                phys_addr = align_address(phys_addr).aligned;

                for (uint32_t i = 0; i < pages; ++i)
                    map_page_locked(virt_addr + i * PAGE_SIZE, phys_addr + i * PAGE_SIZE, flags);

                flush_remote_tlbs();
            }

            static void map_identity_page(uint32_t addr, uint32_t flags) {
                map_page(addr, addr, flags);
            }

            static void map_identity_pages(uint32_t addr, uint32_t pages, uint32_t flags) {
                map_pages(addr, addr, pages, flags);
            }

            static void map_identity_span(uint32_t addr, uint32_t size, uint32_t flags) {
                if (!size)
                    return;

                const AlignedResult aligned = align_address(addr);
                const uint32_t      span    = align_up(aligned.offset + size, PAGE_SIZE);

                map_identity_pages(aligned.aligned, span / PAGE_SIZE, flags);
            }

            static void unmap_page(uint32_t virt_addr) {
                kstd::SpinLockGuard guard(lock_);
                unmap_page_locked(virt_addr);
                flush_remote_tlbs();
            }

            static void unmap_pages(uint32_t virt_addr, uint32_t pages) {
                kstd::SpinLockGuard guard(lock_);
                virt_addr = align_address(virt_addr).aligned;

                for (uint32_t i = 0; i < pages; ++i)
                    unmap_page_locked(virt_addr + i * PAGE_SIZE);

                flush_remote_tlbs();
            }

            static uint32_t virt_to_phys(uint32_t virt_addr) {
                kstd::SpinLockGuard guard(lock_);
                Entry&              pde = pde_entry(virt_addr);
                if (!pde.has_flag(Present))
                    return 0xFFFFFFFF;

                Entry& pte = pte_entry(virt_addr);
                if (!pte.has_flag(Present))
                    return 0xFFFFFFFF;

                return pte.address() | (virt_addr & (PAGE_SIZE - 1));
            }

            static bool is_mapped(uint32_t virt_addr) {
                kstd::SpinLockGuard guard(lock_);
                Entry&              pde = pde_entry(virt_addr);
                if (!pde.has_flag(Present))
                    return false;

                return pte_entry(virt_addr).has_flag(Present);
            }

            static inline void load_directory(uint32_t phys_addr) {
                __asm__ volatile ("mov %0, %%cr3" : : "r"(phys_addr) : "memory");
            }

            static inline void flush_tlb(uint32_t virt_addr) {
                __asm__ volatile ("invlpg (%0)" : : "r"(virt_addr) : "memory");
            }

            static inline void enable_paging() {
                uint32_t cr0;
                __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
                cr0 |= 0x80000000;
                __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
            }

            static inline void flush_current_tlb() {
                load_directory(kernel_dir_phys);
            }

            static void page_fault(uint32_t err_code, idt::BaseInterruptFrame* ctx) {
                uint32_t cr2;
                __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

                const char* err = "Unknown";

                if (!(err_code & 0x1)) {
                    err = "Page not present";
                } else if (err_code & 0x2) {
                    err = "Page is read-only";
                } else if (err_code & 0x4) {
                    err = "Processor in user-mode";
                } else if (err_code & 0x8) {
                    err = "Overwrite CPU-reserved bits";
                }

                kstd::panic("Page fault at 0x%08x (faddr 0x%08x): %s\n",
                    (uint32_t)ctx->eip, cr2, err);
            }

        private:
            inline static kstd::SpinLock lock_;

            static void flush_remote_tlbs();
            static void tlb_shootdown_handler(uint32_t err_code, idt::BaseInterruptFrame* ctx);

            static void ensure_page_table(uint32_t virt_addr, uint32_t flags) {
                Entry& pde = pde_entry(virt_addr);
                if (pde.has_flag(Present)) {
                    pde.update_flags(flags);
                    return;
                }

                const uint32_t pt_phys = pmm::alloc_frame();
                if (!pt_phys)
                    kstd::panic("ensure_page_table: out of physical memory");

                pde.invalidate();
                pde.set_address(pt_phys);
                pde.update_flags(Present | Writable | (flags & User));

                // New page tables come from PMM as raw frames. Clear the recursive
                // mapping view so untouched PTEs cannot inherit stale state.
                memset(reinterpret_cast<uint8_t*>(pt_virtual_base(virt_addr)), 0, PAGE_SIZE);
            }

            static void map_page_locked(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
                virt_addr = align_address(virt_addr).aligned;
                phys_addr = align_address(phys_addr).aligned;

                ensure_page_table(virt_addr, flags);

                Entry& pte = pte_entry(virt_addr);
                pte.invalidate();
                pte.set_address(phys_addr);
                pte.update_flags(flags);

                flush_tlb(virt_addr);
            }

            static void unmap_page_locked(uint32_t virt_addr) {
                virt_addr = align_address(virt_addr).aligned;

                Entry& pde = pde_entry(virt_addr);
                if (!pde.has_flag(Present))
                    return;

                Entry& pte = pte_entry(virt_addr);
                if (!pte.has_flag(Present))
                    return;

                pmm::free_frame(pte.address());
                pte.invalidate();
                flush_tlb(virt_addr);
            }
    };
}
