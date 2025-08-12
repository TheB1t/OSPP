# OSPP (OS++)

## Overview

OSPP (OS++) is a hobbyist kernel project designed to explore low-level system programming using C++17. Built for x86 architectures, it implements a modular monolithic kernel with core functionalities like memory management, interrupt handling, and task scheduling. The project leverages C++'s type safety and abstractions to create a clean, extensible system, debugged and tested on both emulators (QEMU, Bochs) and real hardware.

## Build Requirements

For Debian-based systems, install the following packages:

```bash
sudo apt -y install qemu qemu-system gcc g++ binutils make cmake grub-pc
```

## Build and Run Instructions

1. **Initialize Disk Image** (creates a 32MB disk image with ext2 filesystem and GRUB):

   ```bash
   ./utils.sh init
   ```

2. **Build and Pack Kernel**:

   ```bash
   ./utils.sh pack [Debug|Release]
   ```

3. **Run Options**:

   - QEMU: `./utils.sh run`
   - QEMU with KVM: `./utils.sh runk`
   - Bochs: `./utils.sh runb`
   - Debug with GDB: `./utils.sh rund`

4. **Manual Build**:

   ```bash
   cmake -B build . -DCMAKE_BUILD_TYPE=Debug
   make -C build -j$(nproc)
   ```

   Binaries are output to the `output/` directory.

## Acknowledgments

Inspired by OSDev.org and open-source community resources.