#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/trivial.hpp>

namespace kstd {
    enum class MemoryOrder : int {
        Relaxed = __ATOMIC_RELAXED,
        Consume = __ATOMIC_CONSUME,
        Acquire = __ATOMIC_ACQUIRE,
        Release = __ATOMIC_RELEASE,
        AcqRel  = __ATOMIC_ACQ_REL,
        SeqCst  = __ATOMIC_SEQ_CST,
    };

    inline void atomic_thread_fence(MemoryOrder order = MemoryOrder::SeqCst) {
        __atomic_thread_fence(static_cast<int>(order));
    }

    template<typename T>
    class Atomic : public NonTransferable {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
            "Atomic only supports 1, 2, 4 and 8 byte objects");
        static_assert(__is_trivially_copyable(T), "Atomic requires trivially copyable types");

        public:
            constexpr Atomic() : value_{} {}
            constexpr Atomic(T value) : value_(value) {}

            T load(MemoryOrder order = MemoryOrder::SeqCst) const {
                return __atomic_load_n(&value_, static_cast<int>(order));
            }

            void store(T value, MemoryOrder order = MemoryOrder::SeqCst) {
                __atomic_store_n(&value_, value, static_cast<int>(order));
            }

            T exchange(T value, MemoryOrder order = MemoryOrder::SeqCst) {
                return __atomic_exchange_n(&value_, value, static_cast<int>(order));
            }

            bool compare_exchange_strong(T& expected, T desired,
                MemoryOrder success = MemoryOrder::SeqCst,
                MemoryOrder failure = MemoryOrder::SeqCst) {
                return __atomic_compare_exchange_n(
                    &value_,
                    &expected,
                    desired,
                    false,
                    static_cast<int>(success),
                    static_cast<int>(failure)
                );
            }

        private:
            alignas(sizeof(T)) mutable T value_;
    };

    template<>
    class Atomic<uint32_t> : public NonTransferable {
        public:
            constexpr Atomic() : value_(0) {}
            constexpr Atomic(uint32_t value) : value_(value) {}

            uint32_t load(MemoryOrder order = MemoryOrder::SeqCst) const {
                return __atomic_load_n(&value_, static_cast<int>(order));
            }

            void store(uint32_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                __atomic_store_n(&value_, value, static_cast<int>(order));
            }

            uint32_t exchange(uint32_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                return __atomic_exchange_n(&value_, value, static_cast<int>(order));
            }

            bool compare_exchange_strong(uint32_t& expected, uint32_t desired,
                MemoryOrder success = MemoryOrder::SeqCst,
                MemoryOrder failure = MemoryOrder::SeqCst) {
                return __atomic_compare_exchange_n(
                    &value_,
                    &expected,
                    desired,
                    false,
                    static_cast<int>(success),
                    static_cast<int>(failure)
                );
            }

            uint32_t fetch_add(uint32_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                return __atomic_fetch_add(&value_, value, static_cast<int>(order));
            }

            uint32_t fetch_sub(uint32_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                return __atomic_fetch_sub(&value_, value, static_cast<int>(order));
            }

        private:
            alignas(sizeof(uint32_t)) mutable uint32_t value_;
    };

    template<>
    class Atomic<uint64_t> : public NonTransferable {
        public:
            constexpr Atomic() : value_(0) {}
            constexpr Atomic(uint64_t value) : value_(value) {}

            uint64_t load(MemoryOrder order = MemoryOrder::SeqCst) const {
                return __atomic_load_n(&value_, static_cast<int>(order));
            }

            void store(uint64_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                __atomic_store_n(&value_, value, static_cast<int>(order));
            }

            uint64_t exchange(uint64_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                return __atomic_exchange_n(&value_, value, static_cast<int>(order));
            }

            bool compare_exchange_strong(uint64_t& expected, uint64_t desired,
                MemoryOrder success = MemoryOrder::SeqCst,
                MemoryOrder failure = MemoryOrder::SeqCst) {
                return __atomic_compare_exchange_n(
                    &value_,
                    &expected,
                    desired,
                    false,
                    static_cast<int>(success),
                    static_cast<int>(failure)
                );
            }

            uint64_t fetch_add(uint64_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                return __atomic_fetch_add(&value_, value, static_cast<int>(order));
            }

            uint64_t fetch_sub(uint64_t value, MemoryOrder order = MemoryOrder::SeqCst) {
                return __atomic_fetch_sub(&value_, value, static_cast<int>(order));
            }

        private:
            alignas(sizeof(uint64_t)) mutable uint64_t value_;
    };
}
