# Kernel Features

OSPP (OS++) is a C++17-based monolithic kernel for x86 architectures, designed for modularity and low-level system programming. Below are its current capabilities, leveraging modern C++ for robust and type-safe kernel development.

## Core Features

- **C++17 Integration**:
  - Uses `constexpr` for compile-time computations (e.g., CRC32 table in `crc32.hpp`).
  - Employs templates for type-safe interrupt handling (e.g., ISR wrappers in `idt.hpp`).
  - Includes a custom `klibcpp` library with utilities like `kstd::vector` and `kstd::array` for freestanding environments.

- **Memory Management**:
  - **Physical Memory Manager (PMM)**: Manages physical memory with a bitmap-based allocator, initialized via Multiboot memory maps (`pmm.cpp`).
  - **Virtual Memory Manager (VMM)**: Supports page-based virtual-to-physical mappings (`vmm.cpp`).
  - **Heap**: Provides dynamic memory allocation with alignment support, using a linked-list chunk structure (`heap.cpp`).

- **Interrupt Handling**:
  - **Global Descriptor Table (GDT)**: Defines kernel and user segments with Task State Segment (TSS) for context switching (`gdt.hpp`).
  - **Interrupt Descriptor Table (IDT)**: Manages interrupts and exceptions with templated ISR wrappers (`idt.hpp`).
  - **Programmable Interrupt Controller (PIC)**: Configures hardware interrupts with remapping support (`pic.hpp`).
  - **Programmable Interval Timer (PIT)**: Drives scheduling with configurable intervals (`pit.hpp`).

- **Task Scheduling**:
  - Preemptive scheduler with a 10ms default time slice, managing tasks via `kstd::vector` (`scheduler.cpp`).
  - Supports task creation, context switching, and termination detection.

- **Device Drivers**:
  - **Early Display**: VGA text output for debugging, integrated with a custom `printf` (`early_display.hpp`).
  - **Serial**: Supports multiple COM ports with configurable baud rates and loopback testing (`serial.hpp`).
  - **ACPI and APIC**: Initial support for power management and multi-core interrupt handling (`acpi.cpp`, `apic.cpp`).

- **Multiboot Compliance**:
  - Boots via GRUB using the Multiboot specification for flexible bootloader integration.

- **Symmetric Multiprocessing (SMP)**:
  - Preliminary support for multi-core systems with APIC and trampoline code for core initialization (`smp.cpp`, `smp_ll.cpp`).

- **Error Handling and Debugging**:
  - Robust panic mechanism (`kstd::panic`) for unrecoverable errors.
  - Logging via serial and VGA outputs with `LOG_INFO`/`LOG_WARN` macros (`log.hpp`).
  - Debug symbols for GDB debugging (`utils.sh`).

## Development Tools

- **Compilers**: GCC (C++17).
- **Build System**: CMake with a custom linker script (`link.ld`) for freestanding environments.
- **Debugging**: GDB integration with QEMU and Bochs, supported by VSCode configurations (`launch.json`, `tasks.json`).
- **Emulators**: QEMU (with KVM) and Bochs for testing.
- **Hardware Support**: Tested on HP T5570 Thin Client (VIA Nano U3500, 1GB RAM).

See [TO_IMPLEMENT.md](TO_IMPLEMENT.md) for planned enhancements.