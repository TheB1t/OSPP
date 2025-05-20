/*
    This file contains implementation of
    OS++ virtual memory manager. It using
    recursive mapping approach for manage
    entries.
*/
#pragma once

#include <klibcpp/kstd.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <int/idt.hpp>
#include <mm/pmm.hpp>

/*
    const uint32_t pde_index = (virt >> 22) & 0x3FF;
    const uint32_t pte_index = (virt >> 12) & 0x3FF;
    const uint32_t pt_address = PT_BASE + (pde_index << 12);
*/

namespace mm {
    static constexpr uint32_t PDE_BASE = 0xFFFFF000;
    static constexpr uint32_t PT_BASE  = 0xFFC00000;
    static constexpr uint32_t PAGE_MASK = 0xFFFFF000;

    enum Flags : uint32_t {
        All             = 0,
        Present         = 1 << 0,
        Writable        = 1 << 1,
        User            = 1 << 2,
        WriteThrough    = 1 << 3,
        CacheDisabled   = 1 << 4,
        Accessed        = 1 << 5,
        Dirty           = 1 << 6,
        HugePage        = 1 << 7,
    };

    struct Entry {
        uint32_t value = 0;

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
            return (value & flag & FLAGS_MASK) == flag;
        }

        void update_flags(uint32_t flags) {
            value |= (flags & FLAGS_MASK);
        }

        // Entry& operator=(uint32_t val) {
        //     value = val;
        //     return *this;
        // }
    };

    static_assert(sizeof(Entry) == 4, "Entry size must be 4 bytes");

    struct PDE;
    struct PTE;

    namespace TableEntry {
        static constexpr uint32_t MASK = 0x3FF;

        template<typename T>
        struct Index;

        template<>
        struct Index<PTE> {
            static constexpr uint32_t SHIFT = 12;
            static inline uint32_t index(uint32_t value) {
                return (value >> SHIFT) & MASK;
            }
        };

        template<>
        struct Index<PDE> {
            static constexpr uint32_t SHIFT = 22;
            static inline uint32_t index(uint32_t value) {
                return (value >> SHIFT) & MASK;
            }
        };

        template<typename T>
        struct Accessor;

        template<>
        struct Accessor<PDE> {
            static Entry& get(uint32_t virt_addr) {
                uint32_t pd_index = Index<PDE>::index(virt_addr);
                return *reinterpret_cast<Entry*>(PDE_BASE + pd_index * sizeof(Entry));
            }
        };

        template<>
        struct Accessor<PTE> {
            static Entry& get(uint32_t virt_addr) {
                uint32_t pd_index = Index<PDE>::index(virt_addr);
                Entry& pde = *reinterpret_cast<Entry*>(PDE_BASE + pd_index * sizeof(Entry));

                if (!pde.has_flag(Present)) {
                    serial::printf("--->>>>>>> Found invalid PDE entry, creating new one\n");
                    pde.invalidate();
                    pde.set_address(pmm::alloc_page());
                    pde.update_flags(Present | Writable);
                }

                uint32_t pt_address = PT_BASE + (pd_index << 12);
                uint32_t pt_index = Index<PTE>::index(virt_addr);
                return *reinterpret_cast<Entry*>(pt_address + pt_index * sizeof(Entry));
            }
        };
    };

    struct AlignedResult {
        uint32_t aligned;
        uint32_t offset;
    };

    static AlignedResult align_address(uint32_t address) {
        return {
            address & PAGE_MASK,
            address & (PAGE_SIZE - 1)
        };
    }

    class vmm {
        public:
            static uint32_t kernel_dir_phys;

            static void init() {
                uint32_t pd_phys = pmm::alloc_page();
                Entry* pd = reinterpret_cast<Entry*>(pd_phys);

                uint32_t pt_phys = pmm::alloc_page();
                Entry* pt = reinterpret_cast<Entry*>(pt_phys);

                memset((uint8_t*)pd, 0, PAGE_SIZE);

                for (uint32_t i = 0; i < 1024; i++) {
                    pt[i].invalidate();
                    pt[i].set_address(i * 0x1000);
                    pt[i].update_flags(Present | Writable);
                }

                pd[0].invalidate();
                pd[0].set_address(pt_phys);
                pd[0].update_flags(Present | Writable);

                pd[1023].invalidate();
                pd[1023].set_address(pd_phys);
                pd[1023].update_flags(Present | Writable);

                idt::register_isr(14, page_fault);

                kernel_dir_phys = pd_phys;
                load_directory(kernel_dir_phys);
                enable_paging();
            }

            static void map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
                virt_addr = align_address(virt_addr).aligned;
                phys_addr = align_address(phys_addr).aligned;

                Entry& pde = TableEntry::Accessor<PDE>::get(virt_addr);
                ensure_pde_valid(pde, flags);

                Entry& pte = TableEntry::Accessor<PTE>::get(virt_addr);
                pte.invalidate();
                pte.set_address(phys_addr);
                pte.update_flags(flags);

                flush_tlb(virt_addr);
            }

            static void map_pages(uint32_t virt_addr, uint32_t phys_addr, uint32_t pages, uint32_t flags) {
                phys_addr = align_address(phys_addr).aligned;
                virt_addr = align_address(virt_addr).aligned;

                for (uint32_t i = 0; i < pages; i++)
                    map_page(virt_addr + i * PAGE_SIZE, phys_addr + i * PAGE_SIZE, flags);
            }

            static void unmap_page(uint32_t virt_addr) {
                Entry& pte = TableEntry::Accessor<PTE>::get(virt_addr);

                if(!pte.has_flag(Present))
                    return;

                pmm::free_page(pte.address());
                pte.invalidate(); // Invalidate the PTE

                // Flush page
                flush_tlb(virt_addr);

                // TODO: Flush page table, if it's empty
            }

            static void unmap_pages(uint32_t virt_addr, uint32_t pages) {
                const auto [aligned_virt, offset] = align_address(virt_addr);

                for (uint32_t i = 0; i < pages; i++)
                    unmap_page(aligned_virt + i * PAGE_SIZE);
            }

            static uint32_t virt_to_phys(uint32_t virt_addr) {
                const Entry& pte = TableEntry::Accessor<PTE>::get(virt_addr);
                if (!pte.has_flag(Present))
                    return 0xFFFFFFFF;

                return pte.address() | (virt_addr & (PAGE_SIZE - 1));
            }

            static inline void load_directory(uint32_t phys_addr) {
                INLINE_ASSEMBLY("mov %0, %%cr3" : : "r"(phys_addr) : "memory");
            }

            static inline void flush_tlb(uint32_t virt_addr) {
                INLINE_ASSEMBLY("invlpg (%0)" : : "r"(virt_addr) : "memory");
            }

            static inline void enable_paging() {
                uint32_t cr0;
                INLINE_ASSEMBLY("mov %%cr0, %0" : "=r"(cr0));
                cr0 |= 0x80000000;
                INLINE_ASSEMBLY("mov %0, %%cr0" : : "r" (cr0));
            }

            static void page_fault(uint32_t err_code, bool has_ext, idt::BaseInterruptContext* ctx) {
                uint32_t cr2;
                INLINE_ASSEMBLY("mov %%cr2, %0" : "=r"(cr2));

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

                if (has_ext) {
                    idt::InterruptContext* ext = idt::InterruptContext::from_base(ctx);
                    kstd::stack_trace((kstd::stack_frame*)ext->ebp, 10);
                }

                kstd::panic("Page fault at 0x%08x (faddr 0x%08x): %s\n", (uint32_t)ctx->eip, cr2, err);
            }

        private:
            static void ensure_pde_valid(Entry& pde, uint32_t flags) {
                if (!pde.has_flag(Present)) {
                    pde.invalidate();
                    pde.set_address(pmm::alloc_page());
                    pde.update_flags(Present | Writable);
                }

                pde.update_flags(flags);
            }
    };
}