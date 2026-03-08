#pragma once

#include <klibcpp/atomic.hpp>
#include <klibcpp/iguard.hpp>
#include <klibcpp/trivial.hpp>

namespace kstd {
    class SpinLock : public NonTransferable {
        public:
            constexpr SpinLock() : locked_(false) {}

            void lock() {
                while (locked_.exchange(true, MemoryOrder::Acquire)) {
                    while (locked_.load(MemoryOrder::Relaxed))
                        __pause;
                }
            }

            bool try_lock() {
                return !locked_.exchange(true, MemoryOrder::Acquire);
            }

            void unlock() {
                locked_.store(false, MemoryOrder::Release);
            }

        private:
            Atomic<bool> locked_;
    };

    class SpinLockGuard : public NonTransferable {
        public:
            explicit SpinLockGuard(SpinLock& lock)
                : lock_(lock) {
                lock_.lock();
            }

            ~SpinLockGuard() {
                lock_.unlock();
            }

        private:
            SpinLock& lock_;
    };

    class InterruptSpinLockGuard : public NonTransferable {
        public:
            explicit InterruptSpinLockGuard(SpinLock& lock)
                : interrupt_guard_(), lock_guard_(lock) {}

        private:
            InterruptGuard interrupt_guard_;
            SpinLockGuard lock_guard_;
    };
}
