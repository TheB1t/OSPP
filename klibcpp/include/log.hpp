#pragma once

#include <klibcpp/cstring.hpp>
#include <klibcpp/cstdlib.hpp>

#define _LOG_VA_ARGS(...) , ##__VA_ARGS__
#define LOG(level, format, ...) Log::printf(level, __FILE__, __LINE__, __func__, format _LOG_VA_ARGS(__VA_ARGS__))

#define LOG_INFO(format, ...)   LOG(Log::INFO, format, __VA_ARGS__)
#define LOG_WARN(format, ...)   LOG(Log::WARN, format, __VA_ARGS__)
#define LOG_DEBUG(format, ...)  LOG(Log::DEBUG, format, __VA_ARGS__)
#define LOG_ERR(format, ...)    LOG(Log::ERR, format, __VA_ARGS__)
#define LOG_CRIT(format, ...)   LOG(Log::CRIT, format, __VA_ARGS__)
#define LOG_FATAL(format, ...)  LOG(Log::FATAL, format, __VA_ARGS__)

namespace Log {
    enum LogLevel {
        INFO, WARN, DEBUG, ERR, CRIT, FATAL, ALL
    };

    constexpr static const char* level_str[] = {
        "INFO", "WARN", "DEBUG", "ERR", "CRIT", "FATAL"
    };

    static const LogLevel global_level = Log::ALL;

    template <typename... Args>
    static void printf(LogLevel level, const char* file, int line, const char* func, const char *fmt, const Args&... args) {
        kstd::InterruptGuard guard;

        if (level > global_level)
            return;

        static constexpr size_t BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];
        size_t remaining = BUFFER_SIZE;
        char* pos = buffer;

        auto safe_snprintf = [&](const char* format, auto... params) {
            int written = snprintf(pos, remaining, format, params...);
            if (written < 0 || static_cast<size_t>(written) >= remaining) {
                written = remaining > 0 ? remaining - 1 : 0;
            }
            pos += written;
            remaining -= written;
        };

        if (api.get_ticks)
            safe_snprintf("[%010d]", api.get_ticks());

#if defined(LOG_SHOW_FILE_LINE)
        const char* _file = file;
#if !defined(LOG_SHOW_FILE_FULLPATH)
        const char* last_slash = strrchr(file, '/');
        const char* last_backslash = strrchr(file, '\\');
        if (last_slash || last_backslash) {
            _file = (last_slash > last_backslash) ? last_slash + 1 : last_backslash + 1;
        }
#endif
        safe_snprintf("[%14s:%-6d]", _file, line);
#else
        (void)file;
        (void)line;
#endif

#if defined(LOG_SHOW_FUNC_NAME)
        safe_snprintf("[%-20s]", func);
#else
        (void)func;
#endif

        safe_snprintf("<%6s>: ", level_str[level]);
        safe_snprintf(fmt, args...);

        puts(buffer);
    }
};