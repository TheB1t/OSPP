#include <multiboot.hpp>
#include <icxxabi.hpp>
#include <int/gdt.hpp>
#include <int/idt.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <klibc/cstdlib.hpp>
#include <klibc/cstdint.hpp>
#include <klibc/cstring.hpp>
#include <driver/pic.hpp>
#include <driver/pit.hpp>
#include <driver/early_display.hpp>
#include <driver/serial.hpp>

#include <memory.hpp>

multiboot_info_t mboot_saved;

class TestClass {
    public:
        int a;
        int b;
        int c;

        TestClass() {
            serial::printf("Hello from TestClass constructor (this = 0x%08x)\n", this);
        }

        ~TestClass() {
            serial::printf("Hello from TestClass destructor (this = 0x%08x)\n", this);
        }
};

TestClass test;
shared_ptr<TestClass> class0;

void at_exit_test(void* objptr __unused) {
    serial::printf("Hello from at_exit_test\n");
}

void test_func(shared_ptr<TestClass> ptr) {
    serial::printf("Hello from test_func (ptr = 0x%08x, use_count = %d)\n", ptr.get(), ptr.use_count());
}

void kernel_main(multiboot_info_t* mboot __unused) {
    TestClass test;
    serial::printf("Hello from kernel_main\n");
    __cxa_atexit(at_exit_test, 0, 0);

    class0 = make_shared<TestClass>();
    shared_ptr<TestClass> class1 = make_shared<TestClass>();
    shared_ptr<TestClass> class2(class1);
    class2.reset();

    test_func(class0);

    serial::printf("class0 = 0x%08x, class1 = 0x%08x\n", class0.get(), class1.get());
    serial::printf("class0 use_count = %d, class1 use_count = %d\n", class0.use_count(), class1.use_count());

    serial::printf("We reached end of kernel_main\n");
    __bochs_brk();
}

extern "C" {
    void _init();
    void _fini();

    inline void init_fpu(void) {
        uint32_t cr0, cr4;

        // Read CR0
        INLINE_ASSEMBLY("mov %%cr0, %0" : "=r"(cr0));
        // Clear EM (bit 2), set MP (bit 1)
        cr0 &= ~(1 << 2);  // EM = 0
        cr0 |=  (1 << 1);  // MP = 1
        INLINE_ASSEMBLY("mov %0, %%cr0" :: "r"(cr0));

        // Read CR4
        INLINE_ASSEMBLY("mov %%cr4, %0" : "=r"(cr4));
        // Set OSFXSR (bit 9), OSXMMEXCPT (bit 10) for SSE
        cr4 |= (1 << 9) | (1 << 10);
        INLINE_ASSEMBLY("mov %0, %%cr4" :: "r"(cr4));

        // Initialize FPU
        INLINE_ASSEMBLY("fninit");
    }

    void kernel_early_main(multiboot_info_t* mboot, uint32_t magic) {
        serial::init();
        EarlyDisplay::clear();

        if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
            serial::printf("Invalid bootloader magic number: 0x%08x (should be 0x%08x)\n", magic, MULTIBOOT_BOOTLOADER_MAGIC);
            __unreacheble();
        }

        memcpy((uint8_t*)mboot, (uint8_t*)&mboot_saved, sizeof(multiboot_info_t));

        init_fpu();

        pic::remap();
        pit::init(pit::calculate_pit_divisor_us(1000));

        gdt::init_bsp();
        idt::init();

        __sti();

        mm::pmm::init(&mboot_saved);
        mm::vmm::init();

        kernel_heap.create(0x01000000, HEAP_MIN_SIZE, 0x02000000, mm::Present | mm::Writable);

        _init();
        kernel_main(&mboot_saved);
        __cxa_finalize(0);
        _fini();

        __unreacheble();
    }
}