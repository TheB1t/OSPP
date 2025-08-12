#include <multiboot.hpp>
#include <icxxabi.hpp>
#include <int/gdt.hpp>
#include <int/idt.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <mm/heap.hpp>
#include <klibcpp/cstdlib.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/memory.hpp>
#include <klibabi/kapi.hpp>
#include <driver/pic.hpp>
#include <driver/pit.hpp>
#include <driver/early_display.hpp>
#include <sys/acpi.hpp>
#include <sys/apic.hpp>
#include <sys/module.hpp>
#include <sys/smp.hpp>
#include <sys/kexp.hpp>
#include <sched/scheduler.hpp>
#include <log.hpp>

static multiboot_info_t mboot_saved;
KernelAPI api;

EXPORT_SYMBOL("api", api);

class TestClass {
    public:
        int a;
        int b;
        int c;

        TestClass() {
            LOG_INFO("Hello from TestClass constructor (this = 0x%08x)\n", this);
        }

        ~TestClass() {
            LOG_INFO("Hello from TestClass destructor (this = 0x%08x)\n", this);
        }
};

TestClass test;
kstd::shared_ptr<TestClass> class0;

void at_exit_test() {
    LOG_INFO("Hello from at_exit_test\n");
}

void test_func(kstd::shared_ptr<TestClass> ptr) {
    LOG_INFO("Hello from test_func (ptr = 0x%08x, use_count = %d)\n", ptr.get(), ptr.use_count());
}

void kernel_main(multiboot_info_t*) {
    /*
        At this stage, a near-complete C++ environment is available, allowing
        the use of most C++ features—excluding the STL, of course. Features
        such as `new` and `delete` are supported, with only a minimal risk
        of catastrophic failure — depending on how carefully they are used :D.
    */
    TestClass test;
    LOG_INFO("Hello from kernel_main\n");
    EarlyDisplay::printf("Successfully initialized\n");
    kstd::atexit(at_exit_test);

    class0 = kstd::make_shared<TestClass>();
    kstd::shared_ptr<TestClass> class1 = kstd::make_shared<TestClass>();
    kstd::shared_ptr<TestClass> class2(class1);
    class2.reset();

    test_func(class0);

    LOG_INFO("class0 = 0x%08x, class1 = 0x%08x\n", class0.get(), class1.get());
    LOG_INFO("class0 use_count = %d, class1 use_count = %d\n", class0.use_count(), class1.use_count());

    LOG_INFO("We reached end of kernel_main\n");
    EarlyDisplay::printf("Leaving kernel_main\n");

    sched::Scheduler::get()->create_task("task1", []() {
        while (true) {
            LOG_INFO("Task 1 running\n");
            pit::sleep_us(1000 * 1000);
        }
    });

    sched::Scheduler::get()->create_task("task2", []() {
        LOG_INFO("Task 2 running\n");
    });

    pit::sleep_us(2 * 1000 * 1000);
}

__extern_c {
    void _init();
    void _fini();

    __cdecl void* _kmalloc(uint32_t size, bool align) {
        return (void*)heap_malloc(Heap::get_kernel_heap(), size, align);
    }

    __cdecl void _kfree(void* ptr) {
        heap_free(Heap::get_kernel_heap(), ptr);
    }

    __cdecl uint32_t _get_ticks() {
        return pit::ticks();
    }

    __cdecl void _putc(char c) {
        if (serial::primary.initialized)
            serial::primary.putc(c);
    }

    __cdecl void _panic(const char* s) {
        kstd::_panic(s);
    }

    void init_api() {
        api.kmalloc    = &_kmalloc;
        api.kfree      = &_kfree;
        api.get_ticks  = &_get_ticks;
        api.putc       = &_putc;
        api.panic      = &_panic;
    }

    void kernel_early_main(multiboot_info_t* mboot, uint32_t magic) {
        init_api();

        // TODO: Implement proper FPU initialization
        kstd::init_fpu();

        serial::init();
        EarlyDisplay::clear();

        if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
            LOG_INFO("[boot] Invalid bootloader magic number: 0x%08x (should be 0x%08x)\n", magic, MULTIBOOT_BOOTLOADER_MAGIC);
            __unreachable();
        }

        memcpy((uint8_t*)&mboot_saved, (uint8_t*)mboot, sizeof(multiboot_info_t));

        LOG_INFO("[boot] Loaded %d modules\n", mboot_saved.mods_count);
        LOG_INFO("[boot] Loaded %s table\n",
            mboot_saved.flags & MULTIBOOT_INFO_AOUT_SYMS    ? "symbol" :
            mboot_saved.flags & MULTIBOOT_INFO_ELF_SHDR     ? "section" : "no one"
        );

        pic::remap();
        pit::init(pit::calculate_pit_divisor_us(1000));

        gdt::init_bsp();

        /*
            Above this point, you may only use functionality that does not
            require interrupts and cannot trigger any interrupts
            (e.g., page faults, division by zero, etc.),
            as doing so will result in an immediate triple fault.
        */
        idt::init();

        /*
            Above this point, only exceptions are functional;
            user-defined or hardware interrupts will be ignored.
        */
        __sti();

        mm::pmm::init(&mboot_saved);
        mm::vmm::init();
        acpi::init();

        /*
            Initializing the kernel heap. Attempting to use it before this point
            will result in a page fault or other undefined behavior.
        */
        Heap::get_kernel_heap()->create(0x01000000, HEAP_MIN_SIZE, 0x02000000, mm::Present | mm::Writable);

        multiboot_module_t* mod = (multiboot_module_t*)mboot_saved.mods_addr;
        uint32_t mod_count = mboot_saved.mods_count;

        for (uint32_t i = 0; i < mod_count; i++) {
            ModuleManager::get()->registerModule((void*)mod->mod_start);
        }

        apic::get()->init(); // Initialize APIC
        apic::get()->configure(); // Configure APIC

        pic::disable(); // Disable PIC
        apic::get()->enable(); // Enable APIC

        kstd::trigger_interrupt<50>(); // Test interrupts :)
        // serial::set_interrupts(serial::COM1, true);

        smp::init();

        /*
            You may initialize any components that do not depend on the
            initialization of global or static variables before calling _init().

            However, attempting to initialize objects that *do* rely on
            global or static variables before their initialization will lead to
            undefined behavior — such as a page fault or other critical faults.
        */
        LOG_INFO("Hello from kernel_early_main\n");

        LOG_INFO("[cpp] _init()\n");
        _init();

        sched::Scheduler::get()->init();
        sched::Scheduler::get()->yield();

        kernel_main(&mboot_saved);

        __cli();

        LOG_INFO("[cpp] __cxa_finalize(0)\n");
        __cxa_finalize(0);
        LOG_INFO("[cpp] _fini()\n");
        _fini();

        delete sched::Scheduler::get();
        delete apic::get();
        delete Linker::get();
        delete ModuleManager::get();

        LOG_INFO("Terminated\n");
        __unreachable();
    }
}