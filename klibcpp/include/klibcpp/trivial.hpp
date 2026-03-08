#pragma once

struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(const NonCopyable&)            = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

struct NonMovable {
    NonMovable()                         = default;
    NonMovable(NonMovable&&)             = delete;
    NonMovable&  operator=(NonMovable&&) = delete;
};

struct NonTransferable : public NonCopyable, public NonMovable
{};

struct TrivialDtor {
    ~TrivialDtor() = default;
};