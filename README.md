# OSPP (OS++)

### What is this?

This is a hobby project of mine, where i'm trying to create my own operating system (5th try).

### What tools i using for development

Debugging tools & things:
- qemu
- bochs
- gdb
- real hardware

Compile & build tools:
- gcc
- cmake

#### Hardware list

| Manufacturer | Model | Type        | Chipset   | Processor               | RAM                                 |
| ------------ | ----- | ----------- | --------- | ----------------------- | ----------------------------------- |
| HP           | T5570 | Thin Client | VIA VX900 | VIA Nano U3500, 1000MHz | `1Gb Samsung M471B2873FHS-CH9` x1   |

### Requirements for build and run

1. Install this packages (if you using debian-based system):

```bash
sudo apt -y install qemu qemu-system gcc g++ binutils make cmake grub-pc
```

### How to build/pack/run it

1. Create image (need only once, or if you don't have image, or want to recreate it):

```bash
./utils.sh init
```

2. Build and pack kernel into image:

```bash
./utils.sh pack
```

3. Running
    1. Using `qemu`:

    ```bash
    ./utils.sh run
    ```

    2. Using `qemu` with kvm:

    ```bash
    ./utils.sh runk
    ```

    3. Using `bochs`:

    ```bash
    ./utils.sh runb
    ```

or you can run the build manually using:

```bash
cmake -B build .
make -C build
```

and run as you prefer. Compiled binaries will be located in the `output/bin` folder.

### Contributing

If you want to contribute to this project, you can do so by following these steps:

1. Fork this repo
2. Create a branch
3. Commit your changes
4. Create a pull request
5. Wait for review