#include <klibcpp/printf_impl.hpp>

void _vprintf(BaseOutputter& out, const char* format, va_list args) {
    char c;
    int num = 0;
    int precision = -1;
    char sym = ' ';
    char buf[32];
    bool padRight = false;

    while ((c = *format++) != 0) {
        padRight = false;
        sym = ' ';
        num = 0;
        precision = -1;

        if (c != '%') {
            out.write(c);
        } else {
            c = *format++;
            if (c == 0) break;

        back:
            switch (c) {
                case 'd':
                case 'u':
                case 'x': {
                    int value = va_arg(args, int);
                    if (c == 'x')
                        htoa(value, buf);
                    else
                        itoa(value, buf);
                    char* p = buf;
                    int len = strlen(p);

                    if (num) {
                        int padding = num - len;
                        if (padding > 0 && !padRight) {
                            while (padding--)
                                out.write(sym);
                        }
                    }

                    while (*p)
                        out.write(*p++);

                    if (num) {
                        int padding = num - len;
                        if (padding > 0 && padRight) {
                            while (padding--)
                                out.write(sym);
                        }
                    }
                    break;
                }

                case 's': {
                    char* p = va_arg(args, char*);
                    if (!p)
                        p = (char*)"(null)";
                    int len = strlen(p);

                    if (num) {
                        int padding = num - len;
                        if (padding > 0 && !padRight) {
                            while (padding--)
                                out.write(sym);
                        }
                    }

                    while (*p)
                        out.write(*p++);

                    if (num) {
                        int padding = num - len;
                        if (padding > 0 && padRight) {
                            while (padding--)
                                out.write(sym);
                        }
                    }
                    break;
                }

                case 'c': {
                    int ch = va_arg(args, int);
                    out.write(static_cast<char>(ch));
                    break;
                }

                case 'f': {
                    double value = va_arg(args, double);
                    int prec = (precision == -1) ? 6 : precision;
                    ftoa(value, buf, prec);
                    char* p = buf;
                    int len = strlen(p);

                    if (num) {
                        int padding = num - len;
                        if (padding > 0 && !padRight) {
                            while (padding--)
                                out.write(sym);
                        }
                    }

                    while (*p)
                        out.write(*p++);

                    if (num) {
                        int padding = num - len;
                        if (padding > 0 && padRight) {
                            while (padding--)
                                out.write(sym);
                        }
                    }
                    break;
                }

                default: {
                    if (*(format - 2) == '%') {
                        if (c == '-') {
                            padRight = true;
                            c = *format++;
                        }
                        if (c == '0' || c == ' ') {
                            sym = c;
                            c = *format++;
                        }

                        num = 0;
                        while (c >= '0' && c <= '9') {
                            num = num * 10 + (c - '0');
                            c = *format++;
                        }

                        if (c == '.') {
                            c = *format++;
                            precision = 0;
                            while (c >= '0' && c <= '9') {
                                precision = precision * 10 + (c - '0');
                                c = *format++;
                            }
                        }

                        goto back;
                    } else {
                        out.write(c);
                    }
                    break;
                }
            }
        }
    }
}

void _printf(BaseOutputter& out, const char* format, ...) {
    va_list args;
    va_start(args, format);
    _vprintf(out, format, args);
    va_end(args);
}