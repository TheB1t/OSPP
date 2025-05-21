# Contributing to OSPP (OS++) Kernel

OSPP (OS++) is a C++17-based kernel project welcoming contributions to enhance its low-level system capabilities. Whether you're adding drivers, optimizing memory management, or improving documentation, your efforts help advance this hobbyist kernel. Below are guidelines for effective collaboration.

## How to Contribute

1. **Fork the Repository**: Create your own copy on GitHub.
2. **Create a Branch**: Use a descriptive name, e.g., `git checkout -b feature/add-driver`.
3. **Make Changes**: Write clean, modular C++17 code, adhering to the kernel's freestanding environment and coding style.
4. **Test Your Changes**: Verify functionality using QEMU (`./utils.sh run`) or Bochs (`./utils.sh runb`). Test on real hardware if feasible.
5. **Commit Changes**: Use clear commit messages, e.g., `Add PIT driver enhancements`.
6. **Push to Your Fork**: `git push origin feature/add-driver`.
7. **Open a Pull Request**: Submit your changes for review, including a description of the changes and their purpose.

## Coding Guidelines

- **Language**: Use C++17, leveraging features like `constexpr` and templates for type safety.
- **Freestanding Environment**: Avoid standard library dependencies; use `klibcpp` utilities (e.g., `kstd::vector`, `kstd::array`).
- **Error Handling**: Use `kstd::panic` for unrecoverable errors and `LOG_INFO`/`LOG_WARN` for debugging.
- **Documentation**: Comment complex logic, especially for low-level operations like interrupt handling or memory allocation.
- **Testing**: Ensure changes work in both Debug and Release builds (`./utils.sh pack Debug` or `Release`).

## Areas for Contribution

- **Drivers**: Add support for devices like keyboards or storage controllers.
- **Scheduler**: Enhance task management or implement priority scheduling.
- **Memory Management**: Optimize the heap or virtual memory system.
- **Documentation**: Improve code comments or update `FEATURES.md` and `TO_IMPLEMENT.md`.

## Community

Join discussions on [GitHub Issues](https://github.com/TheB1t/ospp/issues) to propose ideas or report bugs.