#pragma once

#include <klibcpp/cstdarg.hpp>
#include <klibcpp/cstring.hpp>

struct OutputSink {
    void* ctx;
    void  (*putc)(void* ctx, char c);
};

struct BufferCtx {
    char*  buf;
    size_t pos;
};

class BufferOutputter {
    public:
        explicit BufferOutputter(char* buffer) {
            ctx_.buf   = buffer;
            ctx_.pos   = 0;
            sink_.ctx  = &ctx_;
            sink_.putc = [](void* pctx, char c) {
                    auto* ctx = static_cast<BufferCtx*>(pctx);
                    if (!ctx->buf)
                        return;
                    ctx->buf[ctx->pos++] = c;
                };
        }
        ~BufferOutputter() {
            if (ctx_.buf) ctx_.buf[ctx_.pos] = '\0';
        }

        OutputSink& sink()  { return sink_; }
        int get_length() const { return static_cast<int>(ctx_.pos); }

    private:
        BufferCtx ctx_{};
        OutputSink sink_{};
};

struct SizedBufferCtx {
    char*  buf;
    size_t cap;
    size_t written;
    size_t total;
};

class BufferOutputterWithSize {
    public:
        BufferOutputterWithSize(char* buffer, size_t size) {
            ctx_.buf     = buffer;
            ctx_.cap     = size;
            ctx_.written = 0;
            ctx_.total   = 0;
            sink_.ctx    = &ctx_;
            sink_.putc   = [](void* pctx, char c) {
                    auto* ctx = static_cast<SizedBufferCtx*>(pctx);
                    if (ctx->buf && ctx->cap) {
                        if (ctx->written + 1 < ctx->cap) {
                            ctx->buf[ctx->written++] = c;
                        }
                    }
                    ++ctx->total;
                };
        }
        ~BufferOutputterWithSize() {
            if (!ctx_.buf || ctx_.cap == 0) return;
            const size_t term = (ctx_.written < ctx_.cap) ? ctx_.written : (ctx_.cap - 1);
            ctx_.buf[term] = '\0';
        }

        OutputSink& sink()  { return sink_; }
        int get_total()  const { return static_cast<int>(ctx_.total); }
        size_t length()  const { return ctx_.written; }
        size_t size()    const { return ctx_.cap; }

    private:
        SizedBufferCtx ctx_{};
        OutputSink sink_{};
};

void _vprintf(OutputSink& out, const char* format, va_list args);
void _printf(OutputSink& out, const char* format, ...);