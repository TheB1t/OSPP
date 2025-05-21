# Kernel Planned Features

OSPP (OS++) is a C++17-based kernel aiming to serve as the foundation for a future operating system. Below are planned enhancements, divided into **global** (complex, high-effort) and **local** (simpler, quicker to implement) features, sorted by complexity within each category.

## Global Features (Complex)

- **Porting GCC for User-Space Compilation**:
  - Port the GNU Compiler Collection (GCC) to compile C and C++ user-space programs targeting OSPP's architecture.
  - Configure GCC with a custom target triple for OSPP, ensuring compatibility with the kernel's ABI and memory model.
  - Provide a cross-compilation toolchain to simplify development of user-space applications.

- **Porting Lightweight libc for User-Space Applications**:
  - Port a minimal `libc` implementation (e.g., inspired by `musl` or `newlib`) to support user-space applications.
  - Include essential C standard library functions (e.g., `malloc`, `printf`, `strcpy`) tailored for OSPP's freestanding environment.
  - Integrate with the kernel's system call interface to enable user-space memory management and I/O operations.

- **System Call Mechanism for Flexible Extensibility**:
  - Implement a generic system call interface to allow adding new calls without modifying the kernel.
  - Use C++17 features (e.g., `klibcpp` variant-like structures) for type-safe argument handling.
  - Enable scalable support for user-space applications and future `libc` integration.

- **Virtual File System (VFS)**:
  - Implement a VFS layer to abstract filesystem operations, enabling support for multiple filesystem types (e.g., FAT32, ext2).
  - Provide a unified interface for file and directory operations, supporting kernel and future user-space interactions.
  - Design VFS with C++17 abstractions (e.g., classes for nodes, inodes) to ensure modularity and type safety.

- **Modular Driver Loading**:
  - Introduce a mechanism for dynamically loading drivers into the kernel at runtime.
  - Develop a C++ interface (e.g., abstract `Driver` class) to standardize driver implementation.
  - Enhance modularity by allowing driver additions without kernel recompilation.

- **Basic Networking (TCP/IP Stack)**:
  - Implement a minimal TCP/IP stack to support network drivers (e.g., Ethernet).
  - Integrate with future VFS for socket interfaces (e.g., `/dev/net`).
  - Prepare the kernel for networked user-space applications.

- **User-Mode Support**:
  - Introduce system calls to support user-space applications.
  - Implement privilege separation between kernel and user modes.

- **Enhanced SMP Support**:
  - Implement full multi-core scheduling to distribute tasks across CPUs.
  - Optimize APIC configuration for efficient interrupt routing.

- **Filesystem Support**:
  - Add a basic filesystem driver (e.g., FAT32 or ext2) for persistent storage.
  - Enable file read/write operations and directory management.

## Local Features (Simpler or Quicker to Implement)

- **PS/2 Keyboard Support**:
  - Develop a PS/2 keyboard driver to enable basic input for kernel interaction (e.g., simple console).
  - Extend existing driver infrastructure (e.g., `serial.hpp`) for keyboard handling.
  - Improve debugging and early user interaction.

- **Priority-Based Logging**:
  - Extend the logging system (`log.hpp`) with priority levels (e.g., DEBUG, INFO, ERROR).
  - Add runtime log filtering based on priority via kernel configuration.
  - Simplify debugging and performance analysis.

- **HPET Timer Support**:
  - Implement a driver for High Precision Event Timer (HPET) as an alternative to PIT (`pit.hpp`).
  - Use HPET for more accurate task scheduling.
  - Enhance scheduler performance on modern hardware.

- **Expanded Driver Support**:
  - Develop drivers for input devices (e.g., mouse) or additional storage controllers.
  - Extend support for network interfaces as a precursor to the TCP/IP stack.

- **Memory Management Optimization**:
  - Enhance heap performance with better chunk allocation strategies.
  - Improve VMM for faster page table updates and memory protection.

- **Power Management**:
  - Expand ACPI support for advanced power states and device configuration.
  - Implement CPU idle states for energy efficiency.

- **Testing and Stability**:
  - Add unit tests for core components (e.g., heap, scheduler).
  - Conduct stress tests on real hardware to ensure stability.

## How to Contribute

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on contributing to these features. Propose ideas or report progress via [GitHub Issues](https://github.com/TheB1t/ospp/issues).