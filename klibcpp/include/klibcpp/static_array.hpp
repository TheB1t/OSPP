#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/utility.hpp>
#include <klibcpp/slot_iterator.hpp>
#include <klibcpp/static_slot.hpp>
#include <klibcpp/trivial.hpp>

namespace kstd {
    template<typename T, size_t N>
    class StaticArray : public NonTransferable {
        public:
            using iterator       = SlotIterator<StaticArray<T, N>, T>;
            using const_iterator = ConstSlotIterator<StaticArray<T, N>, T>;

            StaticArray() = default;

            ~StaticArray() {
                clear();
            }

            constexpr size_t capacity() const {
                return N;
            }

            size_t live_count() const {
                return live_count_;
            }

            bool empty() const {
                return live_count_ == 0;
            }

            bool full() const {
                return live_count_ == N;
            }

            bool occupied(size_t index) const {
                if (index >= N)
                    return false;

                return slots_[index].constructed();
            }

            T* ptr(size_t index) {
                if (index >= N)
                    return nullptr;

                return slots_[index].ptr_if_constructed();
            }

            const T* ptr(size_t index) const {
                if (index >= N)
                    return nullptr;

                return slots_[index].ptr_if_constructed();
            }

            T& get(size_t index) {
                T* p = ptr(index);
                if (!p)
                    kstd::panic("StaticArray::get: invalid slot");
                return *p;
            }

            const T& get(size_t index) const {
                const T* p = ptr(index);
                if (!p)
                    kstd::panic("StaticArray::get: invalid slot");
                return *p;
            }

            T& operator[](size_t index) {
                return get(index);
            }

            const T& operator[](size_t index) const {
                return get(index);
            }

            template<typename... Args>
            T& emplace(Args&&... args) {
                size_t idx = find_free_slot();
                if (idx >= N)
                    kstd::panic("StaticArray full");

                T& obj = slots_[idx].construct(kstd::forward<Args>(args)...);
                ++live_count_;
                return obj;
            }

            template<typename... Args>
            T& emplace_at(size_t index, Args&&... args) {
                if (index >= N)
                    kstd::panic("StaticArray::emplace_at: index out of range");

                if (slots_[index].constructed())
                    kstd::panic("StaticArray::emplace_at: slot already occupied");

                T& obj = slots_[index].construct(kstd::forward<Args>(args)...);
                ++live_count_;
                return obj;
            }

            bool erase(size_t index) {
                if (index >= N)
                    return false;

                if (!slots_[index].constructed())
                    return false;

                slots_[index].destruct();
                --live_count_;
                return true;
            }

            void clear() {
                for (size_t i = 0; i < N; ++i) {
                    if (slots_[i].constructed())
                        slots_[i].destruct();
                }

                live_count_ = 0;
            }

            size_t index_of(const T* obj) const {
                if (!obj)
                    return npos;

                for (size_t i = 0; i < N; ++i) {
                    const T* p = slots_[i].ptr_if_constructed();
                    if (p == obj)
                        return i;
                }

                return npos;
            }

            bool contains(const T* obj) const {
                return index_of(obj) != npos;
            }

            size_t first_free_slot() const {
                for (size_t i = 0; i < N; ++i) {
                    if (!slots_[i].constructed())
                        return i;
                }

                return npos;
            }

            iterator begin() {
                return iterator(this, 0);
            }

            iterator end() {
                return iterator(this, N);
            }

            const_iterator begin() const {
                return const_iterator(this, 0);
            }

            const_iterator end() const {
                return const_iterator(this, N);
            }

            const_iterator cbegin() const {
                return const_iterator(this, 0);
            }

            const_iterator cend() const {
                return const_iterator(this, N);
            }

            static constexpr size_t npos = static_cast<size_t>(-1);

        private:
            StaticSlot<T> slots_[N];
            size_t live_count_ = 0;

            size_t find_free_slot() const {
                for (size_t i = 0; i < N; ++i) {
                    if (!slots_[i].constructed())
                        return i;
                }

                return npos;
            }
    };
}