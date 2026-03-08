#pragma once

#include <klibcpp/cstdlib.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/trivial.hpp>

template<
    uint32_t BaseAddr,
    uint32_t MaxUnits,
    uint32_t UnitSize,
    typename AddrT = uint32_t
>
class FixedBitmapAllocator : public NonTransferable {
    public:
        static_assert(MaxUnits > 0, "MaxUnits must be > 0");
        static_assert(UnitSize > 0, "UnitSize must be > 0");

        static constexpr uint32_t BASE_ADDR   = BaseAddr;
        static constexpr uint32_t MAX_UNITS   = MaxUnits;
        static constexpr uint32_t UNIT_SIZE   = UnitSize;
        static constexpr uint32_t BITMAP_SIZE = (MaxUnits + 31u) / 32u;

        constexpr FixedBitmapAllocator()
            : bitmap{}, allocated_units_(0) {}

        void clear() {
            memset(reinterpret_cast<uint8_t*>(bitmap), 0, sizeof(bitmap));
            allocated_units_ = 0;
        }

        void set() {
            memset(reinterpret_cast<uint8_t*>(bitmap), 0xFF, sizeof(bitmap));
            allocated_units_ = MAX_UNITS;
        }

        AddrT alloc_units(uint32_t count) {
            if (count == 0)
                return 0;

            if (count > MAX_UNITS)
                kstd::panic("alloc_units: request too large");

            const uint32_t last = MAX_UNITS - count;

            for (uint32_t i = 0; i <= last; ++i) {
                bool found = true;

                for (uint32_t j = 0; j < count; ++j) {
                    if (test_bit(i + j)) {
                        found = false;
                        i    += j;
                        break;
                    }
                }

                if (found) {
                    for (uint32_t j = 0; j < count; ++j)
                        set_bit(i + j);

                    allocated_units_ += count;
                    return static_cast<AddrT>(BASE_ADDR + i * UNIT_SIZE);
                }
            }

            kstd::panic("alloc_units: out of memory");
            return 0;
        }

        void free_units(AddrT base, uint32_t count) {
            if (count == 0)
                return;

            if (base < BASE_ADDR)
                kstd::panic("free_units: address below base");

            const uint32_t offset = static_cast<uint32_t>(base - BASE_ADDR);

            if ((offset % UNIT_SIZE) != 0)
                kstd::panic("free_units: unaligned address");

            const uint32_t start = offset / UNIT_SIZE;

            if (start + count > MAX_UNITS)
                kstd::panic("free_units: invalid range");

            for (uint32_t i = 0; i < count; ++i) {
                if (!test_bit(start + i))
                    kstd::panic("free_units: double free");

                clear_bit(start + i);
            }

            allocated_units_ -= count;
        }

        AddrT alloc_unit() {
            return alloc_units(1);
        }

        void free_unit(AddrT addr) {
            free_units(addr, 1);
        }

        void mark_units_free(AddrT base, uint32_t count) {
            if (count == 0)
                return;

            if (base < BASE_ADDR)
                kstd::panic("mark_units_free: address below base");

            const uint32_t offset = static_cast<uint32_t>(base - BASE_ADDR);

            if ((offset % UNIT_SIZE) != 0)
                kstd::panic("mark_units_free: unaligned address");

            const uint32_t start = offset / UNIT_SIZE;

            if (start + count > MAX_UNITS)
                kstd::panic("mark_units_free: invalid range");

            for (uint32_t i = 0; i < count; ++i) {
                if (test_bit(start + i)) {
                    clear_bit(start + i);
                    --allocated_units_;
                }
            }
        }

        void mark_units_used(AddrT base, uint32_t count) {
            if (count == 0)
                return;

            if (base < BASE_ADDR)
                kstd::panic("mark_units_used: address below base");

            const uint32_t offset = static_cast<uint32_t>(base - BASE_ADDR);

            if ((offset % UNIT_SIZE) != 0)
                kstd::panic("mark_units_used: unaligned address");

            const uint32_t start = offset / UNIT_SIZE;

            if (start + count > MAX_UNITS)
                kstd::panic("mark_units_used: invalid range");

            for (uint32_t i = 0; i < count; ++i) {
                if (!test_bit(start + i)) {
                    set_bit(start + i);
                    ++allocated_units_;
                }
            }
        }

        bool contains(AddrT addr) const {
            if (addr < BASE_ADDR)
                return false;

            const uint32_t offset = static_cast<uint32_t>(addr - BASE_ADDR);
            return (offset / UNIT_SIZE) < MAX_UNITS;
        }

        bool allocated(AddrT addr) const {
            if (!contains(addr))
                return false;

            const uint32_t offset = static_cast<uint32_t>(addr - BASE_ADDR);

            if ((offset % UNIT_SIZE) != 0)
                return false;

            return test_bit(offset / UNIT_SIZE);
        }

        uint32_t allocated_unit_count() const {
            return allocated_units_;
        }

        uint32_t free_unit_count() const {
            return MAX_UNITS - allocated_units_;
        }

    private:
        uint32_t bitmap[BITMAP_SIZE];
        uint32_t allocated_units_;

        void set_bit(uint32_t bit) {
            bitmap[bit / 32u] |= (1u << (bit % 32u));
        }

        void clear_bit(uint32_t bit) {
            bitmap[bit / 32u] &= ~(1u << (bit % 32u));
        }

        bool test_bit(uint32_t bit) const {
            return (bitmap[bit / 32u] & (1u << (bit % 32u))) != 0;
        }
};
