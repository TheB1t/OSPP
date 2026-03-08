#include <kernel.hpp>

#include <icxxabi.hpp>
#include <int/gdt.hpp>
#include <int/idt.hpp>
#include <mm/layout.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <klibcpp/cstdlib.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/memory.hpp>
#include <klibabi/kapi.hpp>
#include <driver/pic.hpp>
#include <driver/pit.hpp>
#include <driver/early_display.hpp>
#include <driver/serial.hpp>
#include <sys/acpi.hpp>
#include <sys/kexp.hpp>
#include <multiboot_utils.hpp>
#include <log.hpp>

#if defined(KERNEL_SELF_TESTS)
#include <ktest/builtin.hpp>
#endif

__extern_c {
    void _init();
    void _fini();
}

KernelAPI api;

EXPORT_SYMBOL("api", api);

static Kernel* g_kernel = nullptr;
alignas(Kernel) static uint8_t kernel_storage[sizeof(Kernel)];

void Kernel::init(multiboot_info_t* mboot) {
    memcpy((uint8_t*)&_mboot, (uint8_t*)mboot, sizeof(multiboot_info_t));

    LOG_INFO("[boot] Loaded %d modules\n", multiboot::module_count(_mboot));
    LOG_INFO("[boot] Loaded %s table\n",
        _mboot.flags & MULTIBOOT_INFO_AOUT_SYMS    ? "symbol" :
        _mboot.flags & MULTIBOOT_INFO_ELF_SHDR     ? "section" : "no one"
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

    mm::pmm::init(&_mboot);
    mm::vmm::init();
    acpi::init();

    /*
        Initializing the kernel heap. Attempting to use it before this point
        will result in a page fault or other undefined behavior.
    */
    constexpr const auto& heap_region = mm::layout::virt::region<mm::layout::virt::RegionId::KernelHeap>();
    _heap.construct(heap_region.base, HEAP_MIN_SIZE, static_cast<uint32_t>(heap_region.end()),
        mm::Present | mm::Writable);

    _linker.construct();
    _mmanager.construct(_linker.ptr_if_constructed());
    multiboot::for_each_module(_mboot, [&](uint32_t, const multiboot_module_t& module) {
            _mmanager->registerModule(reinterpret_cast<void*>(module.mod_start));
        });

    _apic.construct();
    _apic->init();
    _apic->configure();


    _cmanager.construct(this, reinterpret_cast<uint32_t>(_apic->lapic_base), _apic->lapics);
    _cmanager->init_bsp();

    pic::disable();
    _apic->get_lapic().enable();

    /*
        Above this point, only exceptions are functional;
        user-defined or hardware interrupts will be ignored.
    */
    __sti();
    _cmanager->init();

    /*
        You may initialize any components that do not depend on the
        initialization of global or static variables before calling _init().

        However, attempting to initialize objects that *do* rely on
        global or static variables before their initialization will lead to
        undefined behavior — such as a page fault or other critical faults.
    */
    LOG_INFO("[cpp] _init()\n");
    _init();

    _sched.construct();
    _sched->yield();
}

Kernel::~Kernel() {
    __cli();

    LOG_INFO("[cpp] __cxa_finalize(0)\n");
    __cxa_finalize(0);

#if defined(KERNEL_SELF_TESTS)
    ktest::builtin::finalize();
#endif

    LOG_INFO("[cpp] _fini()\n");
    _fini();

    _sched.try_destruct();
    _cmanager.try_destruct();
    _apic.try_destruct();
    _mmanager.try_destruct();
    _linker.try_destruct();
    _heap.try_destruct();
}

void kernel_main() {
    /*
        At this stage, a near-complete C++ environment is available, allowing
        the use of most C++ features—excluding the STL, of course. Features
        such as `new` and `delete` are supported, with only a minimal risk
        of catastrophic failure — depending on how carefully they are used :D.
    */
    EarlyDisplay::printf("Successfully initialized\n");

#if defined(KERNEL_SELF_TESTS)
    ktest::builtin::run_all(*g_kernel);
#endif

    EarlyDisplay::printf("Leaving kernel_main\n");
}

__extern_c {
    __cdecl void* _kmalloc(uint32_t size, bool align) {
        return (void*)heap_malloc(&g_kernel->_heap.get(), size, align);
    }

    __cdecl void _kfree(void* ptr) {
        heap_free(&g_kernel->_heap.get(), ptr);
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
        api.kmalloc   = &_kmalloc;
        api.kfree     = &_kfree;
        api.get_ticks = &_get_ticks;
        api.putc      = &_putc;
        api.panic     = &_panic;
    }

    void kernel_early_main(multiboot_info_t* mboot, uint32_t magic) {
        stack.init_stack_anchor(0, nullptr);

        g_kernel = new (kernel_storage) Kernel();

        init_api();

        // TODO: Implement proper FPU initialization
        kstd::init_fpu();

        serial::init();
        EarlyDisplay::clear();

        if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
            LOG_INFO("[boot] Invalid bootloader magic number: 0x%08x (should be 0x%08x)\n", magic,
                MULTIBOOT_BOOTLOADER_MAGIC);
            __unreachable();
        }

        g_kernel->init(mboot);

        kernel_main();

        g_kernel->~Kernel();

        LOG_INFO("[boot] Terminated\n");
        __unreachable();
    }
}
