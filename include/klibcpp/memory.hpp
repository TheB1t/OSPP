#pragma once

#include <klibcpp/kstd.hpp>
#include <klibcpp/cstdint.hpp>

namespace kstd {
    template<typename T>
    class shared_ptr {
        public:
            shared_ptr() : block(nullptr) {}

            explicit shared_ptr(T* ptr) {
                block = new ptr_block{ptr, 1};
            }

            shared_ptr(const shared_ptr& other) {
                block = other.block;
                if (block)
                    ++block->refcount;
            }

            shared_ptr& operator=(const shared_ptr& other) {
                if (this != &other) {
                    release();
                    block = other.block;
                    if (block)
                        ++block->refcount;
                }
                return *this;
            }

            ~shared_ptr() {
                release();
            }

            T& operator*() const {
                return *(block ? block->ptr : nullptr);
            }

            T* operator->() const {
                return block ? block->ptr : nullptr;
            }

            T* get() const {
                return block ? block->ptr : nullptr;
            }

            uint32_t use_count() const {
                return block ? block->refcount : 0;
            }

            void reset() {
                release();
                block = nullptr;
            }

            void reset(T* ptr) {
                release();
                if (ptr)
                    block = new ptr_block{ptr, 1};
                else
                    block = nullptr;
            }

        private:
            struct ptr_block {
                T* ptr;
                uint32_t refcount;
            };

            ptr_block* block;

            void release() {
                if (block) {
                    if (--block->refcount == 0) {
                        delete block->ptr;
                        delete block;
                    }
                }
            }
    };

    template<typename T, typename... Args>
    static shared_ptr<T> make_shared(Args&&... args) {
        return shared_ptr<T>(new T(kstd::forward<Args>(args)...));
    }
}