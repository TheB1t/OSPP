#include <klibcpp/printf_impl.hpp>

static void put_padding(OutputSink& out, char sym, int padding) {
    while (padding-- > 0)
        out.putc(out.ctx, sym);
}

static void put_string(OutputSink& out, const char* str, int width, bool padRight, char sym) {
    if (!str)
        str = "(null)";

    const int len = (int)strlen(str);
    const int padding = width > len ? width - len : 0;

    if (!padRight)
        put_padding(out, sym, padding);

    while (*str)
        out.putc(out.ctx, *str++);

    if (padRight)
        put_padding(out, ' ', padding);
}

void _vprintf(OutputSink& out, const char* format, va_list args) {
    char c;
    char buf[64];

    while ((c = *format++) != 0) {
        if (c != '%') {
            out.putc(out.ctx, c);
            continue;
        }

        if (*format == '\0')
            break;

        if (*format == '%') {
            out.putc(out.ctx, *format++);
            continue;
        }

        bool padRight = false;
        char sym = ' ';
        int width = 0;
        int precision = -1;

        if (*format == '-') {
            padRight = true;
            ++format;
        }

        if (*format == '0' || *format == ' ') {
            sym = *format;
            ++format;
        }

        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            ++format;
        }

        if (*format == '.') {
            precision = 0;
            ++format;

            while (*format >= '0' && *format <= '9') {
                precision = precision * 10 + (*format - '0');
                ++format;
            }
        }

        c = *format++;
        switch (c) {
            case 'd': {
                itoa(va_arg(args, int), buf);
                put_string(out, buf, width, padRight, sym);
                break;
            }

            case 'u': {
                utoa(va_arg(args, unsigned int), buf);
                put_string(out, buf, width, padRight, sym);
                break;
            }

            case 'x': {
                htoa(va_arg(args, unsigned int), buf);
                put_string(out, buf, width, padRight, sym);
                break;
            }

            case 'p': {
                buf[0] = '0';
                buf[1] = 'x';
                htoa((uint32_t)va_arg(args, void*), buf + 2);
                put_string(out, buf, width, padRight, sym);
                break;
            }

            case 's': {
                const char* str = va_arg(args, const char*);
                put_string(out, str, width, padRight, sym);
                break;
            }

            case 'c': {
                buf[0] = (char)va_arg(args, int);
                buf[1] = '\0';
                put_string(out, buf, width, padRight, sym);
                break;
            }

            case 'f': {
                const int prec = precision == -1 ? 6 : precision;
                ftoa((float)va_arg(args, double), buf, prec);
                put_string(out, buf, width, padRight, sym);
                break;
            }

            default:
                out.putc(out.ctx, '%');
                out.putc(out.ctx, c);
                break;
        }
    }
}

void _printf(OutputSink& out, const char* format, ...) {
    va_list args;
    va_start(args, format);
    _vprintf(out, format, args);
    va_end(args);
}
