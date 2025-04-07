#pragma once

#include <klibcpp/type_traits.hpp>

template <typename Derived, typename T>
class RegisterBase {
    public:
        // Read value from derived
        T value() const {
            return static_cast<const Derived*>(this)->read();
        }

        // Write value to derived
        void value(T val) {
            static_cast<Derived*>(this)->write(val);
        }

        // Assignment
        RegisterBase& operator=(T val) {
            value(val);
            return *this;
        }

        // Implicit conversion to value type
        operator T() const {
            return value();
        }

        // Arithmetic operators
        RegisterBase& operator+=(T rhs) { value(value() + rhs); return *this; }
        RegisterBase& operator-=(T rhs) { value(value() - rhs); return *this; }
        RegisterBase& operator*=(T rhs) { value(value() * rhs); return *this; }
        RegisterBase& operator/=(T rhs) { value(value() / rhs); return *this; }
        RegisterBase& operator%=(T rhs) { value(value() % rhs); return *this; }

        // Bitwise operators
        RegisterBase& operator|=(T rhs)  { value(value() | rhs); return *this; }
        RegisterBase& operator&=(T rhs)  { value(value() & rhs); return *this; }
        RegisterBase& operator^=(T rhs)  { value(value() ^ rhs); return *this; }
        RegisterBase& operator<<=(T rhs) { value(value() << rhs); return *this; }
        RegisterBase& operator>>=(T rhs) { value(value() >> rhs); return *this; }

        // Unary operators
        T operator~() const { return ~value(); }
        T operator-() const { return -value(); }
        T operator+() const { return +value(); }

        // Pre/post increment/decrement
        RegisterBase& operator++() { value(value() + 1); return *this; }
        RegisterBase operator++(int) { T tmp = value(); value(tmp + 1); return tmp; }

        RegisterBase& operator--() { value(value() - 1); return *this; }
        RegisterBase operator--(int) { T tmp = value(); value(tmp - 1); return tmp; }
};