#include <driver/serial.hpp>

serial::Port serial::COM1 = { 0x3F8, false };
serial::Port serial::COM2 = { 0x2F8, false };
serial::Port serial::COM3 = { 0x3E8, false };
serial::Port serial::COM4 = { 0x2E8, false };
serial::Port serial::COM5 = { 0x5F8, false };
serial::Port serial::COM6 = { 0x4F8, false };
serial::Port serial::COM7 = { 0x5E8, false };
serial::Port serial::COM8 = { 0x4E8, false };

serial::Port& serial::primary = serial::COM1;