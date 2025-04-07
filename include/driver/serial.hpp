#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <io/ports.hpp>
#include <printf_impl.hpp>

class serial {
    public:
        struct BaudRate {
            uint8_t high;
            uint8_t low;
        };

        static constexpr BaudRate Baud115200 = { 0x00, 0x01 };
        static constexpr BaudRate Baud57600 = { 0x00, 0x02 };
        static constexpr BaudRate Baud38400 = { 0x00, 0x03 };
        static constexpr BaudRate Baud19200 = { 0x00, 0x06 };
        static constexpr BaudRate Baud9600 = { 0x00, 0x0C };
        static constexpr BaudRate Baud7200 = { 0x00, 0x10 };
        static constexpr BaudRate Baud4800 = { 0x00, 0x18 };
        static constexpr BaudRate Baud3600 = { 0x00, 0x20 };
        static constexpr BaudRate Baud2400 = { 0x00, 0x30 };
        static constexpr BaudRate Baud2000 = { 0x00, 0x3A };
        static constexpr BaudRate Baud1800 = { 0x00, 0x40 };
        static constexpr BaudRate Baud1200 = { 0x00, 0x60 };
        static constexpr BaudRate Baud600 = { 0x00, 0xC0 };

        enum class Reg {
            DAR = 0,    // Data Register (R/W)
            IER = 1,    // Interrupt Enable
            FCR = 2,    // FIFO Control
            LCR = 3,    // Line Control
            MCR = 4,    // Modem Control
            LSR = 5,    // Line Status
            MSR = 6,    // Modem Status
            SCR = 7,    // Scratch

            DL_LOW = 0, // Divisor Latch Low (when DLAB=1)
            DL_HIGH = 1 // Divisor Latch High (when DLAB=1)
        };

        enum LCRFlags {
            BIT5 = 0b00000000,
            BIT6 = 0b00000001,
            BIT7 = 0b00000010,
            BIT8 = 0b00000011,

            SBIT = 0b00000100,

            ODD  = 0b00001000,
            EVEN = 0b00011000,
            MARK = 0b00101000,
            SPACE = 0b00111000,

            DLAB = 0b10000000,
        };

        enum IERFlags {
            RDA = 0b00000001,
            THRE = 0b00000010,
            RLS = 0b00000100,
            MS = 0b00001000,
        };

        enum FCRFlags {
            ENABLE_FIFO = 0b00000001,
            CLEAR_RFIFO = 0b00000010,
            CLEAR_TFIFO = 0b00000100,
            DMA_MODE = 0b00001000,

            TL_1BYTE = 0b00000000,
            TL_4BITE = 0b01000000,
            TL_8BYTE = 0b10000000,
            TL_14BYTE = 0b11000000,
        };

        enum MCRFlags {
            DTR = 0b00000001,
            RTS = 0b00000010,
            OUT1 = 0b00000100,
            OUT2 = 0b00001000,
            LOOP = 0b00010000,
        };

        struct SerialPort {
            uint16_t address;
            bool initialized;

            inline ports::Port8 operator[](Reg reg) const {
                return ports::Port8{ static_cast<uint16_t>(address + static_cast<uint8_t>(reg)) };
            }

            inline bool exists() const {
                return (*this)[Reg::LSR] != 0xFF;
            }

            inline bool is_transmit_empty() const {
                return (*this)[Reg::LSR] & 0x20;
            }

            inline bool is_receive_ready() const {
                return (*this)[Reg::LSR] & 0x01;
            }

            char getc() const {
                uint32_t timeout = 1000000;
                while (!is_receive_ready() && --timeout)
                    asm volatile("pause");

                return timeout ? (char)(*this)[Reg::DAR] : '\0';
            }

            void putc(char c) const {
                uint32_t timeout = 1000000;
                while (!is_transmit_empty() && --timeout)
                    asm volatile("pause");

                if (timeout) {
                    (*this)[Reg::DAR] = c;
                    if (c == '\n')
                        putc('\r');
                }
            }
        };

        static SerialPort COM1;
        static SerialPort COM2;
        static SerialPort COM3;
        static SerialPort COM4;
        static SerialPort COM5;
        static SerialPort COM6;
        static SerialPort COM7;
        static SerialPort COM8;

        static SerialPort& primary;

        static void init() {
            SerialPort* ports[] = { &COM1, &COM2, &COM3, &COM4, &COM5, &COM6, &COM7, &COM8 };

            for (auto* port : ports)
                init(*port, Baud115200);
        }

        static void init(SerialPort& port, BaudRate baud) {
            if (!port.exists() || port.initialized)
                return;

            set_interrupts(port, false);

            port[Reg::LCR] = DLAB;
            port[Reg::DL_LOW] = baud.low;
            port[Reg::DL_HIGH] = baud.high;
            port[Reg::LCR] = BIT8;
            port[Reg::FCR] = ENABLE_FIFO | CLEAR_RFIFO | CLEAR_TFIFO | TL_14BYTE;
            port[Reg::MCR] = LOOP;

            port.putc('A');
            port.initialized = (port.getc() == 'A'); // Loopback test

            port[Reg::MCR] = DTR | RTS | OUT1 | OUT2;

            printf("[serial] Port 0x%04x %s\n", port.address, (port.initialized ? "initialized" : "failed to initialize"));
        }

        static void set_interrupts(SerialPort& port, bool enable) {
            port[Reg::IER] = enable ? RDA | THRE | RLS | MS : 0;
            port[Reg::FCR] = port[Reg::FCR] | CLEAR_RFIFO;
        }

        class SerialOutputter : public BaseOutputter {
            virtual void write(char c) override {
                if (primary.initialized)
                    primary.putc(c);
            }
        };

        template<typename ...Args>
        static void printf(const char* format, Args&&... args) {
            static SerialOutputter outputter;
            _printf(outputter, format, args...);
        }
};