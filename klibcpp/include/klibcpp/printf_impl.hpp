#pragma once

#include <klibcpp/cstdarg.hpp>
#include <klibcpp/cstring.hpp>

class BaseOutputter {
    public:
        virtual void write(char c) = 0;
};

class BufferOutputter : public BaseOutputter {
    public:
        BufferOutputter(char* buffer) : buf(buffer), pos(0) {}

        void write(char c) override {
            buf[pos++] = c;
        }

        ~BufferOutputter() {
            buf[pos] = '\0';
        }

        int get_length() const {
            return static_cast<int>(pos);
        }

    private:
        char* buf;
        size_t pos;
};

class BufferOutputterWithSize : public BaseOutputter {
    public:
        BufferOutputterWithSize(char* buffer, size_t size)
            : buf(buffer), capacity(size), written(0), total(0) {
            if (capacity == 0) {
                buf = nullptr;
            }
        }

        void write(char c) override {
            ++total;
            if (buf && capacity > 0) {
                if (written < capacity - 1) {
                    buf[written++] = c;
                }
            }
        }

        ~BufferOutputterWithSize() {
            if (buf && capacity > 0) {
                if (written >= capacity) {
                    buf[capacity - 1] = '\0';
                } else {
                    buf[written] = '\0';
                }
            }
        }

        int get_total() const {
            return static_cast<int>(total);
        }

    private:
        char* buf;
        size_t capacity;
        size_t written;
        size_t total;
};

void _vprintf(BaseOutputter& out, const char* format, va_list args);
void _printf(BaseOutputter& out, const char* format, ...);