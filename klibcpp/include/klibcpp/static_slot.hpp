#pragma once

#include <klibcpp/cstdlib.hpp>
#include <klibcpp/utility.hpp>
#include <klibcpp/trivial.hpp>

namespace kstd {

    template <typename T>
    class StaticSlot : public NonTransferable {
        public:
            StaticSlot() : ptr_(nullptr), constructed_(false) {}

            template <typename... Args>
            T& construct(Args&&... args) {
                if (constructed_) {
                    panic("StaticSlot::construct(): already constructed");
                }

                ptr_         = new (storage()) T(static_cast<Args&&>(args)...);
                constructed_ = true;
                return *ptr_;
            }

            template <typename... Args>
            T& reconstruct(Args&&... args) {
                if (constructed_) {
                    destruct();
                }
                return construct(static_cast<Args&&>(args)...);
            }

            void destruct() {
                if (!constructed_) {
                    panic("StaticSlot::destruct(): not constructed");
                }

                ptr_->~T();
                ptr_         = nullptr;
                constructed_ = false;
            }

            bool try_destruct() {
                if (!constructed_) {
                    return false;
                }

                ptr_->~T();
                ptr_         = nullptr;
                constructed_ = false;
                return true;
            }

            T& get() {
                if (!constructed_) {
                    panic("StaticSlot::get(): not constructed");
                }
                return *ptr_;
            }

            const T& get() const {
                if (!constructed_) {
                    panic("StaticSlot::get() const: not constructed");
                }
                return *ptr_;
            }

            T* operator->() {
                return &get();
            }

            const T* operator->() const {
                return &get();
            }

            T& operator*() {
                return get();
            }

            const T& operator*() const {
                return get();
            }

            T* ptr_if_constructed() {
                return constructed_ ? ptr_ : nullptr;
            }

            const T* ptr_if_constructed() const {
                return constructed_ ? ptr_ : nullptr;
            }

            bool constructed() const {
                return constructed_;
            }

        private:
            void* storage() {
                return static_cast<void*>(storage_);
            }

            const void* storage() const {
                return static_cast<const void*>(storage_);
            }

        private:
            alignas(T) unsigned char storage_[sizeof(T)];
            T* ptr_;
            bool constructed_;
    };
}