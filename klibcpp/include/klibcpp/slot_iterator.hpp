#pragma once

#include <klibcpp/cstdlib.hpp>

namespace kstd {
    template<typename Container, typename T>
    class SlotIterator {
        public:
            SlotIterator(Container* owner, size_t index)
                : owner_(owner), index_(index) {
                skip_invalid();
            }

            T& operator*() const {
                return *owner_->ptr(index_);
            }

            T* operator->() const {
                return owner_->ptr(index_);
            }

            SlotIterator& operator++() {
                ++index_;
                skip_invalid();
                return *this;
            }

            bool operator==(const SlotIterator& other) const {
                return owner_ == other.owner_ && index_ == other.index_;
            }

            bool operator!=(const SlotIterator& other) const {
                return !(*this == other);
            }

            size_t index() const {
                return index_;
            }

        private:
            Container* owner_;
            size_t index_;

            void skip_invalid() {
                while (index_ < owner_->capacity() && !owner_->occupied(index_))
                    ++index_;
            }
    };

    template<typename Container, typename T>
    class ConstSlotIterator {
        public:
            ConstSlotIterator(const Container* owner, size_t index)
                : owner_(owner), index_(index) {
                skip_invalid();
            }

            const T& operator*() const {
                return *owner_->ptr(index_);
            }

            const T* operator->() const {
                return owner_->ptr(index_);
            }

            ConstSlotIterator& operator++() {
                ++index_;
                skip_invalid();
                return *this;
            }

            bool operator==(const ConstSlotIterator& other) const {
                return owner_ == other.owner_ && index_ == other.index_;
            }

            bool operator!=(const ConstSlotIterator& other) const {
                return !(*this == other);
            }

            size_t index() const {
                return index_;
            }

        private:
            const Container* owner_;
            size_t index_;

            void skip_invalid() {
                while (index_ < owner_->capacity() && !owner_->occupied(index_))
                    ++index_;
            }
    };
}