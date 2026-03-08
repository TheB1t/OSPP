#pragma once

#include <klibcpp/cstdint.hpp>
#include <mm/pmm.hpp>

namespace mm {
    namespace layout {
        namespace boot {
            inline constexpr uint32_t TRAMPOLINE_BASE = 0x00008000;
            inline constexpr uint32_t ACPI_SCAN_BASE  = 0x000E0000;
            inline constexpr uint32_t ACPI_SCAN_END   = 0x00100000;
            inline constexpr uint32_t LOW_USABLE_BASE = 0x00100000;
        }

        namespace virt {
            enum class RegionId : uint8_t {
                BootstrapIdentity,
                KernelHeap,
                ModuleSpace,
                RecursivePageTables,
            };

            enum class MapKind : uint8_t {
                Fixed,
                Identity,
                Reserved,
                Pool,
                Randomized,
            };

            struct RegionDesc {
                RegionId    id;
                const char* name;
                uint32_t    base;
                uint32_t    size;
                MapKind     kind;

                constexpr uint64_t end() const {
                    return static_cast<uint64_t>(base) + size;
                }

                constexpr bool contains(uint32_t addr) const {
                    return addr >= base && static_cast<uint64_t>(addr) < end();
                }

                constexpr bool contains(uint32_t addr, uint32_t span) const {
                    return span <= size &&
                           addr >= base &&
                           (static_cast<uint64_t>(addr) + span) <= end();
                }
            };

            inline constexpr uint32_t   IDENTITY_WINDOW_BASE     = 0x00000000;
            inline constexpr uint32_t   IDENTITY_WINDOW_SIZE     = 0x00400000;

            inline constexpr uint32_t   KERNEL_HEAP_BASE         = 0x01000000;
            inline constexpr uint32_t   KERNEL_HEAP_SIZE         = 0x01000000;
            inline constexpr uint32_t   KERNEL_HEAP_INITIAL_SIZE = 0x00100000;

            inline constexpr uint32_t   MODULE_SPACE_BASE        = 0x0A000000;
            inline constexpr uint32_t   MODULE_SPACE_SIZE        = 0x00080000;
            inline constexpr uint32_t   MODULE_SPACE_PAGE_COUNT  = MODULE_SPACE_SIZE / PAGE_SIZE;

            inline constexpr uint32_t   RECURSIVE_PT_BASE        = 0xFFC00000;
            inline constexpr uint32_t   RECURSIVE_PT_SIZE        = 0x00400000;
            inline constexpr uint32_t   RECURSIVE_PD_BASE        = RECURSIVE_PT_BASE + RECURSIVE_PT_SIZE - PAGE_SIZE;

            inline constexpr RegionDesc REGIONS[]                = {
                {RegionId::BootstrapIdentity, "bootstrap-identity", IDENTITY_WINDOW_BASE, IDENTITY_WINDOW_SIZE,
                 MapKind::Identity},
                {RegionId::KernelHeap, "kernel-heap", KERNEL_HEAP_BASE, KERNEL_HEAP_SIZE, MapKind::Pool},
                {RegionId::ModuleSpace, "module-space", MODULE_SPACE_BASE, MODULE_SPACE_SIZE, MapKind::Pool},
                {RegionId::RecursivePageTables, "recursive-page-tables", RECURSIVE_PT_BASE, RECURSIVE_PT_SIZE,
                 MapKind::Reserved},
            };

            inline constexpr size_t     REGION_COUNT = sizeof(REGIONS) / sizeof(REGIONS[0]);

            template<RegionId Id>
            struct RegionIndex;

            template<>
            struct RegionIndex<RegionId::BootstrapIdentity> {
                static constexpr size_t value = 0;
            };

            template<>
            struct RegionIndex<RegionId::KernelHeap> {
                static constexpr size_t value = 1;
            };

            template<>
            struct RegionIndex<RegionId::ModuleSpace> {
                static constexpr size_t value = 2;
            };

            template<>
            struct RegionIndex<RegionId::RecursivePageTables> {
                static constexpr size_t value = 3;
            };

            constexpr bool is_page_aligned(uint32_t value) {
                return (value & (PAGE_SIZE - 1)) == 0;
            }

            constexpr bool overlaps(const RegionDesc& lhs, const RegionDesc& rhs) {
                return lhs.base < rhs.end() && rhs.base < lhs.end();
            }

            constexpr bool valid_region(const RegionDesc& region) {
                return region.size > 0 &&
                       is_page_aligned(region.base) &&
                       is_page_aligned(region.size) &&
                       region.end() > static_cast<uint64_t>(region.base);
            }

            constexpr bool validate() {
                for (size_t i = 0; i < REGION_COUNT; ++i) {
                    if (!valid_region(REGIONS[i]))
                        return false;

                    for (size_t j = i + 1; j < REGION_COUNT; ++j) {
                        if (overlaps(REGIONS[i], REGIONS[j]))
                            return false;
                    }
                }

                return true;
            }

            constexpr const RegionDesc* find_region(RegionId id) {
                for (size_t i = 0; i < REGION_COUNT; ++i) {
                    if (REGIONS[i].id == id)
                        return &REGIONS[i];
                }

                return nullptr;
            }

            constexpr const RegionDesc* find_region(uint32_t addr) {
                for (size_t i = 0; i < REGION_COUNT; ++i) {
                    if (REGIONS[i].contains(addr))
                        return &REGIONS[i];
                }

                return nullptr;
            }

            template<RegionId Id>
            constexpr const RegionDesc& region() {
                return REGIONS[RegionIndex<Id>::value];
            }

            constexpr bool occupied(uint32_t addr) {
                return find_region(addr) != nullptr;
            }

            static_assert(KERNEL_HEAP_INITIAL_SIZE <= KERNEL_HEAP_SIZE);
            static_assert(RECURSIVE_PT_SIZE == (1024u * PAGE_SIZE));
            static_assert(RECURSIVE_PD_BASE == 0xFFFFF000);
            static_assert(MODULE_SPACE_PAGE_COUNT == (MODULE_SPACE_SIZE / PAGE_SIZE));
            static_assert(validate(), "virtual memory layout must be page-aligned and non-overlapping");
        }
    }
}
