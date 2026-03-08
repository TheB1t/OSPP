# OSPP
> *stands for* `Oh Shit Plus Plus`, *stylised as* `OS++`

OSPP is a 32-bit x86 hobby kernel written in freestanding C++17. The tree currently contains:

- `kernel/`: the kernel image, bootstrap code, MM, interrupts, SMP, scheduler, ACPI/APIC, and the in-kernel self-test layer
- `klibcpp/`: the freestanding support library used by both the kernel and loadable modules
- `modules/`: relocatable kernel modules linked as `.ko` objects and loaded by the kernel at boot

The project targets `i386`, boots through Multiboot + GRUB, runs under QEMU/Bochs, and is primarily developed against the `q35` machine model.

## Current State

The kernel currently brings up:

- GDT and IDT
- PMM, recursive-paging VMM, and kernel heap
- ACPI table discovery
- LAPIC and IOAPIC setup
- BSP/AP SMP bring-up
- a simple scheduler driven by PIT callbacks
- loadable module linking and entry dispatch
- optional built-in runtime and compile-time self-tests

This is still a bring-up kernel, not a stable OS. A lot of subsystems are real, but many policies are intentionally minimal.

## Repository Layout

```text
.
├── kernel/     # kernel image, linker script, low-level architecture code
├── klibcpp/    # freestanding C++ support layer
├── modules/    # sample/test modules linked as relocatable objects
├── output/     # build artifacts copied here by the build scripts
├── build/      # CMake build directory
└── utils.sh    # image/build/run helper script
```

## Toolchain Requirements

You need a 32-bit capable GCC/binutils toolchain and the host tools used by `utils.sh`.

Typical Linux requirements:

- `gcc`, `g++`, `binutils`, `make`, `cmake`
- `qemu-system-i386` or `qemu-system-x86`
- `grub-install`
- `parted`, `losetup`, `kpartx`
- `mkfs.ext2`
- `objcopy`, `strip`
- `sudo`

Optional:

- `bochs` for Bochs runs
- `gdb` for remote debugging with `./utils.sh rund`
- `uncrustify` for repository formatting

## Build

Manual build:

```bash
cmake -B build . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Artifacts are written to `output/`:

- `output/kernel`
- `output/kernel.debug`
- `output/modules/*.ko`

## Disk Image Workflow

The helper script manages the disk image and GRUB installation.

Initialize a fresh disk image once:

```bash
./utils.sh init
```

Build and pack the kernel plus modules into `disk.img`:

```bash
./utils.sh pack Debug
```

`init` and `pack` require `sudo` because they create loop devices, filesystems, and install GRUB.

## Run

Available run modes:

- `./utils.sh run` - QEMU, serial on stdio
- `./utils.sh runk` - QEMU with KVM enabled
- `./utils.sh rund` - QEMU paused with GDB stub (`-s -S`)
- `./utils.sh runb` - Bochs

The kernel logs to the serial port. Current successful runs reach:

```text
[boot] Terminated
```

## Built-In Self Tests

Kernel self-tests are enabled by default through:

```cmake
OPTION(KERNEL_SELF_TESTS "Enable built-in kernel self tests" ON)
```

They cover:

- freestanding runtime helpers
- static containers and allocators
- PMM/VMM/heap invariants
- scheduler smoke tests
- shutdown-time checks
- compile-time layout and ABI assertions

To disable them:

```bash
cmake -B build . -DCMAKE_BUILD_TYPE=Debug -DKERNEL_SELF_TESTS=OFF
```

## Modules

Modules are linked as relocatable ELF objects and packed into `/modules` inside the disk image. At boot the kernel iterates over the Multiboot module list, links each module into the module address window, and looks up:

- `module_enter`
- `module_exit`

The sample module lives in:

- `modules/test/src/main.cpp`

## Formatting

The repository uses `uncrustify`:

```bash
./utils.sh format
```

The formatting profile is stored in:

- `.uncrustify.cfg`

## Notes

- The kernel is low-linked and identity-maps the early bootstrap window.
- Virtual layout constants are centralized in `kernel/include/mm/layout.hpp`.
- Some early boot and MM assumptions are specific to the current x86 bring-up path.
