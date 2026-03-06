#pragma once

#include <klibcpp/cstdlib.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/utility.hpp>

namespace kstd {
    template <typename T>
    class vector {
        private:
            T* data_;
            size_t size_;
            size_t capacity_;

            void init() {
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;
            }

        public:
            vector() { init(); };

            explicit vector(size_t initial_capacity) {
                init();
                reserve(initial_capacity);
            }

            ~vector() {
                clear();
                if (data_) {
                    ::operator delete(data_, align_val_t(alignof(T)));
                    data_ = nullptr;
                }
            }

            vector(const vector& other) {
                init();
                reserve(other.size_);
                for (size_t i = 0; i < other.size_; ++i) {
                    push_back(other.data_[i]);
                }
            }

            vector& operator=(const vector& other) {
                if (this != &other) {
                    clear();
                    reserve(other.size_);
                    for (size_t i = 0; i < other.size_; ++i) {
                        push_back(other.data_[i]);
                    }
                }
                return *this;
            }

            vector(vector&& other) noexcept
                : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }

            vector& operator=(vector&& other) noexcept {
                if (this != &other) {
                    clear();
                    if (data_) {
                        ::operator delete(data_, align_val_t(alignof(T)));
                    }

                    data_ = other.data_;
                    size_ = other.size_;
                    capacity_ = other.capacity_;

                    other.data_ = nullptr;
                    other.size_ = 0;
                    other.capacity_ = 0;
                }
                return *this;
            }

            T& operator[](size_t index) {
                return data_[index];
            }

            const T& operator[](size_t index) const {
                return data_[index];
            }

            T& at(size_t index) {
                // if (index >= size_)
                    // kstd::throwx(new kstd::BaseException("Bad index"));

                return data_[index];
            }

            const T& at(size_t index) const {
                // if (index >= size_)
                    // kstd::throwx(new kstd::BaseException("Bad index"));

                return data_[index];
            }

            T& front() {
                return data_[0];
            }

            const T& front() const {
                return data_[0];
            }

            T& back() {
                return data_[size_ - 1];
            }

            const T& back() const {
                return data_[size_ - 1];
            }

            T* data() {
                return data_;
            }

            const T* data() const {
                return data_;
            }

            bool empty() const {
                return size_ == 0;
            }

            size_t size() const {
                return size_;
            }

            size_t capacity() const {
                return capacity_;
            }

            void reserve(size_t new_capacity) {
                if (new_capacity <= capacity_) return;

                void* raw = ::operator new(new_capacity * sizeof(T),
                                        align_val_t(alignof(T)));
                // if (!raw)
                //     kstd::panic("vector::reserve OOM");

                T* new_data = static_cast<T*>(raw);

                for (size_t i = 0; i < size_; ++i) {
                    new (&new_data[i]) T(kstd::move(data_[i]));
                    data_[i].~T();
                }

                if (data_) {
                    ::operator delete(data_, align_val_t(alignof(T)));
                }

                data_ = new_data;
                capacity_ = new_capacity;
            }

            void resize(size_t new_size) {
                if (new_size > capacity_)
                    reserve(new_size);

                if (new_size > size_) {
                    for (size_t i = size_; i < new_size; ++i)
                        new (&data_[i]) T();
                } else if (new_size < size_) {
                    for (size_t i = size_; i > new_size; --i)
                        data_[i-1].~T();
                }
                size_ = new_size;
            }

            void clear() {
                for (size_t i = 0; i < size_; ++i)
                    data_[i].~T();
                size_ = 0;
            }

            void push_back(const T& value) {
                if (size_ >= capacity_)
                    reserve(capacity_ == 0 ? 4 : capacity_ * 2);
                new (&data_[size_++]) T(value);
            }

            void push_back(T&& value) {
                if (size_ >= capacity_)
                    reserve(capacity_ == 0 ? 4 : capacity_ * 2);
                new (&data_[size_++]) T(kstd::move(value));
            }

            template<typename... Args>
            void emplace_back(Args&&... args) {
                if (size_ >= capacity_)
                    reserve(capacity_ == 0 ? 4 : capacity_ * 2);

                new (&data_[size_++]) T(kstd::forward<Args>(args)...);
            }

            void pop_back() {
                if (size_ > 0)
                    data_[--size_].~T();
            }

            T* begin() { return data_; }
            T* end()   { return data_ + size_; }
            const T* begin() const { return data_; }
            const T* end()   const { return data_ + size_; }
    };
}
