#pragma once

#include <kernel.hpp>

#include <driver/pit.hpp>
#include <klibcpp/bitmap.hpp>
#include <klibcpp/kstd.hpp>
#include <klibcpp/memory.hpp>
#include <klibcpp/static_array.hpp>
#include <klibcpp/static_slot.hpp>
#include <ktest/compile_time.hpp>
#include <ktest/engine.hpp>

namespace ktest {
    namespace builtin {
        struct State {
            bool     ran_immediate      = false;
            bool     ran_finalize       = false;
            bool     atexit_registered  = false;

            uint32_t global_ctor_count  = 0;
            uint32_t global_dtor_count  = 0;
            uint32_t local_ctor_count   = 0;
            uint32_t local_dtor_count   = 0;
            uint32_t raw_ctor_count     = 0;
            uint32_t raw_dtor_count     = 0;
            uint32_t shared_ctor_count  = 0;
            uint32_t shared_dtor_count  = 0;
            uint32_t slot_ctor_count    = 0;
            uint32_t slot_dtor_count    = 0;
            uint32_t array_ctor_count   = 0;
            uint32_t array_dtor_count   = 0;
            uint32_t atexit_calls       = 0;
            uint32_t task1_runs         = 0;
            uint32_t task2_runs         = 0;

            bool     task1_finished     = false;
            bool     task2_finished     = false;
        };

        inline State& state() {
            static State value;
            return value;
        }

        struct GlobalLifetimeProbe {
            GlobalLifetimeProbe() {
                ++state().global_ctor_count;
            }

            ~GlobalLifetimeProbe() {
                ++state().global_dtor_count;
            }
        };

        struct LocalLifetimeProbe {
            uint32_t magic = 0xC0FFEE00;

            LocalLifetimeProbe() {
                ++state().local_ctor_count;
            }

            ~LocalLifetimeProbe() {
                ++state().local_dtor_count;
            }
        };

        struct RawLifetimeProbe {
            uint32_t magic;

            explicit RawLifetimeProbe(uint32_t magic)
                : magic(magic) {
                ++state().raw_ctor_count;
            }

            ~RawLifetimeProbe() {
                ++state().raw_dtor_count;
            }
        };

        struct SlotProbe {
            uint32_t value;

            explicit SlotProbe(uint32_t value)
                : value(value) {
                ++state().slot_ctor_count;
            }

            ~SlotProbe() {
                ++state().slot_dtor_count;
            }
        };

        struct ArrayProbe {
            uint32_t value;

            explicit ArrayProbe(uint32_t value)
                : value(value) {
                ++state().array_ctor_count;
            }

            ~ArrayProbe() {
                ++state().array_dtor_count;
            }
        };

        struct SharedLifetimeProbe {
            uint32_t value;

            explicit SharedLifetimeProbe(uint32_t value)
                : value(value) {
                ++state().shared_ctor_count;
            }

            ~SharedLifetimeProbe() {
                ++state().shared_dtor_count;
            }
        };

        inline GlobalLifetimeProbe global_lifetime_probe;

        inline void on_atexit() {
            ++state().atexit_calls;
        }

        inline void task1_entry() {
            ++state().task1_runs;
            pit::sleep_us(25 * 1000);
            ++state().task1_runs;
            state().task1_finished = true;
        }

        inline void task2_entry() {
            ++state().task2_runs;
            state().task2_finished = true;
        }

        inline uint32_t low_boot_reserved_end(const Kernel& kernel) {
            uint32_t reserved_end = mm::align_up(reinterpret_cast<uint32_t>(&__kernel_end), mm::PAGE_SIZE);

            if (TEST_MASK(kernel._mboot.flags, MULTIBOOT_INFO_MODS)) {
                auto* modules = reinterpret_cast<const multiboot_module_t*>(kernel._mboot.mods_addr);
                for (uint32_t i = 0; i < kernel._mboot.mods_count; ++i) {
                    const uint32_t mod_end = mm::align_up(modules[i].mod_end, mm::PAGE_SIZE);
                    if (mod_end > reserved_end)
                        reserved_end = mod_end;
                }
            }

            return reserved_end;
        }

        inline uint32_t find_fresh_test_page_base() {
            for (uint32_t addr = 0x04000000; addr < 0xF0000000; addr += 0x00400000) {
                if (!mm::pde_entry(addr).has_flag(mm::Present))
                    return addr;
            }

            kstd::panic("ktest: unable to find unmapped PDE for VMM tests");
            return 0;
        }

        inline void run_runtime_suite(ktest::Session& sess) {
            sess.begin_suite("runtime");

            run_case(sess, "global-constructor", [&]() {
                    KTEST_EXPECT(sess, state().global_ctor_count == 1);
                    KTEST_EXPECT(sess, state().global_dtor_count == 0);
                });

            run_case(sess, "local-lifetime", [&]() {
                    const uint32_t ctor_before = state().local_ctor_count;
                    const uint32_t dtor_before = state().local_dtor_count;

                    {
                        LocalLifetimeProbe probe;
                        KTEST_EXPECT(sess, probe.magic == 0xC0FFEE00);
                    }

                    KTEST_EXPECT(sess, state().local_ctor_count == ctor_before + 1);
                    KTEST_EXPECT(sess, state().local_dtor_count == dtor_before + 1);
                });

            run_case(sess, "raw-new-delete", [&]() {
                    const uint32_t ctor_before = state().raw_ctor_count;
                    const uint32_t dtor_before = state().raw_dtor_count;

                    RawLifetimeProbe* probe = new RawLifetimeProbe(0x12345678);
                    KTEST_ASSERT(sess, probe);
                    KTEST_EXPECT(sess, probe->magic == 0x12345678);

                    delete probe;

                    KTEST_EXPECT(sess, state().raw_ctor_count == ctor_before + 1);
                    KTEST_EXPECT(sess, state().raw_dtor_count == dtor_before + 1);
                });

            run_case(sess, "shared-ptr", [&]() {
                    const uint32_t ctor_before = state().shared_ctor_count;
                    const uint32_t dtor_before = state().shared_dtor_count;

                    auto ptr0 = kstd::make_shared<SharedLifetimeProbe>(0xCAFEBABE);
                    KTEST_ASSERT(sess, ptr0.get() != nullptr);
                    KTEST_EXPECT(sess, ptr0->value == 0xCAFEBABE);
                    KTEST_EXPECT(sess, ptr0.use_count() == 1);

                    auto ptr1 = ptr0;
                    KTEST_EXPECT(sess, ptr0.use_count() == 2);
                    KTEST_EXPECT(sess, ptr1.use_count() == 2);

                    ptr1.reset();
                    KTEST_EXPECT(sess, ptr0.use_count() == 1);
                    KTEST_EXPECT(sess, state().shared_dtor_count == dtor_before);

                    ptr0.reset();
                    KTEST_EXPECT(sess, state().shared_ctor_count == ctor_before + 1);
                    KTEST_EXPECT(sess, state().shared_dtor_count == dtor_before + 1);
                });

            run_case(sess, "shared-ptr-edge-cases", [&]() {
                    const uint32_t ctor_before = state().shared_ctor_count;
                    const uint32_t dtor_before = state().shared_dtor_count;

                    kstd::shared_ptr<SharedLifetimeProbe> empty;
                    KTEST_EXPECT(sess, empty.get() == nullptr);
                    KTEST_EXPECT(sess, empty.use_count() == 0);

                    empty.reset(new SharedLifetimeProbe(0xABCD1234));
                    KTEST_ASSERT(sess, empty.get() != nullptr);
                    KTEST_EXPECT(sess, empty.use_count() == 1);
                    KTEST_EXPECT(sess, empty->value == 0xABCD1234);

                    auto moved = kstd::move(empty);
                    KTEST_EXPECT(sess, empty.get() == nullptr);
                    KTEST_EXPECT(sess, empty.use_count() == 0);
                    KTEST_EXPECT(sess, moved.get() != nullptr);
                    KTEST_EXPECT(sess, moved.use_count() == 1);

                    moved.reset(nullptr);
                    KTEST_EXPECT(sess, moved.get() == nullptr);
                    KTEST_EXPECT(sess, moved.use_count() == 0);
                    KTEST_EXPECT(sess, state().shared_ctor_count == ctor_before + 1);
                    KTEST_EXPECT(sess, state().shared_dtor_count == dtor_before + 1);
                });

            run_case(sess, "atexit-registration", [&]() {
                    state().atexit_calls      = 0;
                    state().atexit_registered = true;

                    kstd::atexit(on_atexit);

                    KTEST_EXPECT(sess, state().atexit_calls == 0);
                });

            sess.end_suite();
        }

        inline void run_container_suite(ktest::Session& sess) {
            sess.begin_suite("containers");

            run_case(sess, "static-slot", [&]() {
                    const uint32_t ctor_before = state().slot_ctor_count;
                    const uint32_t dtor_before = state().slot_dtor_count;

                    kstd::StaticSlot<SlotProbe> slot;
                    KTEST_EXPECT(sess, !slot.constructed());

                    SlotProbe& first = slot.construct(0x11);
                    KTEST_EXPECT(sess, slot.constructed());
                    KTEST_EXPECT(sess, &slot.get() == &first);
                    KTEST_EXPECT(sess, slot.ptr_if_constructed() == &first);
                    KTEST_EXPECT(sess, slot.get().value == 0x11);

                    SlotProbe& second = slot.reconstruct(0x22);
                    KTEST_EXPECT(sess, &slot.get() == &second);
                    KTEST_EXPECT(sess, slot.get().value == 0x22);
                    KTEST_EXPECT(sess, state().slot_ctor_count == ctor_before + 2);
                    KTEST_EXPECT(sess, state().slot_dtor_count == dtor_before + 1);

                    KTEST_EXPECT(sess, slot.try_destruct());
                    KTEST_EXPECT(sess, !slot.constructed());
                    KTEST_EXPECT(sess, !slot.try_destruct());
                    KTEST_EXPECT(sess, state().slot_dtor_count == dtor_before + 2);
                });

            run_case(sess, "static-array", [&]() {
                    const uint32_t ctor_before = state().array_ctor_count;
                    const uint32_t dtor_before = state().array_dtor_count;

                    kstd::StaticArray<ArrayProbe, 4> arr;

                    KTEST_EXPECT(sess, arr.capacity() == 4);
                    KTEST_EXPECT(sess, arr.empty());
                    KTEST_EXPECT(sess, arr.first_free_slot() == 0);

                    ArrayProbe& p0 = arr.emplace(10);
                    ArrayProbe& p3 = arr.emplace_at(3, 30);
                    ArrayProbe& p1 = arr.emplace(20);

                    KTEST_EXPECT(sess, arr.live_count() == 3);
                    KTEST_EXPECT(sess, arr.contains(&p0));
                    KTEST_EXPECT(sess, arr.contains(&p1));
                    KTEST_EXPECT(sess, arr.contains(&p3));
                    KTEST_EXPECT(sess, arr.index_of(&p0) == 0);
                    KTEST_EXPECT(sess, arr.index_of(&p1) == 1);
                    KTEST_EXPECT(sess, arr.index_of(&p3) == 3);
                    KTEST_EXPECT(sess, arr.first_free_slot() == 2);

                    uint32_t sum   = 0;
                    uint32_t count = 0;
                    for (auto& probe : arr) {
                        sum += probe.value;
                        ++count;
                    }

                    KTEST_EXPECT(sess, count == 3);
                    KTEST_EXPECT(sess, sum == 60);

                    KTEST_EXPECT(sess, arr.erase(1));
                    KTEST_EXPECT(sess, !arr.occupied(1));
                    KTEST_EXPECT(sess, arr.live_count() == 2);

                    arr.clear();
                    KTEST_EXPECT(sess, arr.empty());
                    KTEST_EXPECT(sess, state().array_ctor_count == ctor_before + 3);
                    KTEST_EXPECT(sess, state().array_dtor_count == dtor_before + 3);
                });

            run_case(sess, "static-array-edge-cases", [&]() {
                    const uint32_t ctor_before = state().array_ctor_count;
                    const uint32_t dtor_before = state().array_dtor_count;
                    constexpr size_t npos = kstd::StaticArray<ArrayProbe, 4>::npos;

                    kstd::StaticArray<ArrayProbe, 4> arr;

                    KTEST_EXPECT(sess, !arr.erase(0));
                    KTEST_EXPECT(sess, arr.ptr(0) == nullptr);
                    KTEST_EXPECT(sess, arr.ptr(5) == nullptr);
                    KTEST_EXPECT(sess, !arr.occupied(5));
                    KTEST_EXPECT(sess, arr.index_of(nullptr) == npos);

                    ArrayProbe& p0 = arr.emplace_at(0, 1);
                    ArrayProbe& p2 = arr.emplace_at(2, 2);
                    ArrayProbe& p3 = arr.emplace_at(3, 3);

                    auto it = arr.begin();
                    KTEST_EXPECT(sess, it.index() == 0);
                    KTEST_EXPECT(sess, (*it).value == 1);

                    ++it;
                    KTEST_EXPECT(sess, it.index() == 2);
                    KTEST_EXPECT(sess, it->value == 2);

                    const auto& carr = arr;
                    auto cit = carr.begin();
                    KTEST_EXPECT(sess, cit.index() == 0);
                    ++cit;
                    KTEST_EXPECT(sess, cit.index() == 2);

                    KTEST_EXPECT(sess, arr.erase(0));
                    it = arr.begin();
                    KTEST_EXPECT(sess, it.index() == 2);

                    ArrayProbe& p1 = arr.emplace_at(1, 4);
                    (void)p0;
                    (void)p1;
                    (void)p2;
                    (void)p3;
                    KTEST_EXPECT(sess, !arr.full());
                    KTEST_EXPECT(sess, arr.first_free_slot() == 0);
                    KTEST_EXPECT(sess, arr.live_count() == 3);

                    ArrayProbe& p0_again = arr.emplace_at(0, 5);
                    (void)p0_again;
                    KTEST_EXPECT(sess, arr.full());
                    KTEST_EXPECT(sess, arr.first_free_slot() == npos);
                    KTEST_EXPECT(sess, arr.live_count() == 4);

                    arr.clear();
                    KTEST_EXPECT(sess, arr.empty());
                    KTEST_EXPECT(sess, state().array_ctor_count == ctor_before + 5);
                    KTEST_EXPECT(sess, state().array_dtor_count == dtor_before + 5);
                });

            run_case(sess, "bitmap-allocator", [&]() {
                    using TestBitmap = FixedBitmapAllocator<0x400000, 32, 0x1000>;

                    TestBitmap alloc;

                    KTEST_EXPECT(sess, alloc.allocated_unit_count() == 0);
                    KTEST_EXPECT(sess, alloc.free_unit_count() == 32);
                    KTEST_EXPECT(sess, alloc.contains(0x400000));
                    KTEST_EXPECT(sess, alloc.contains(0x41F000));
                    KTEST_EXPECT(sess, !alloc.contains(0x420000));

                    uint32_t a = alloc.alloc_unit();
                    uint32_t b = alloc.alloc_units(2);

                    KTEST_EXPECT(sess, a == 0x400000);
                    KTEST_EXPECT(sess, b == 0x401000);
                    KTEST_EXPECT(sess, alloc.allocated(a));
                    KTEST_EXPECT(sess, alloc.allocated(b));
                    KTEST_EXPECT(sess, alloc.allocated(b + 0x1000));
                    KTEST_EXPECT(sess, alloc.allocated_unit_count() == 3);

                    alloc.free_unit(a);
                    KTEST_EXPECT(sess, !alloc.allocated(a));

                    alloc.mark_units_used(0x404000, 1);
                    KTEST_EXPECT(sess, alloc.allocated(0x404000));

                    alloc.mark_units_free(0x404000, 1);
                    KTEST_EXPECT(sess, !alloc.allocated(0x404000));

                    alloc.free_units(b, 2);
                    KTEST_EXPECT(sess, alloc.allocated_unit_count() == 0);

                    alloc.set();
                    KTEST_EXPECT(sess, alloc.free_unit_count() == 0);
                    alloc.clear();
                    KTEST_EXPECT(sess, alloc.free_unit_count() == 32);
                });

            run_case(sess, "bitmap-allocator-edge-cases", [&]() {
                    using TestBitmap = FixedBitmapAllocator<0x800000, 8, 0x1000>;

                    TestBitmap alloc;

                    KTEST_EXPECT(sess, alloc.alloc_units(0) == 0);
                    alloc.free_units(0x800000, 0);
                    alloc.mark_units_used(0x800000, 0);
                    alloc.mark_units_free(0x800000, 0);

                    KTEST_EXPECT(sess, !alloc.contains(0x7FF000));
                    KTEST_EXPECT(sess, !alloc.contains(0x808000));
                    KTEST_EXPECT(sess, !alloc.allocated(0x800123));

                    alloc.set();
                    KTEST_EXPECT(sess, alloc.allocated_unit_count() == 8);
                    alloc.mark_units_free(0x802000, 2);
                    KTEST_EXPECT(sess, alloc.free_unit_count() == 2);

                    uint32_t run = alloc.alloc_units(2);
                    KTEST_EXPECT(sess, run == 0x802000);
                    KTEST_EXPECT(sess, alloc.allocated(run));
                    KTEST_EXPECT(sess, alloc.allocated(run + 0x1000));

                    alloc.free_units(run, 2);
                    KTEST_EXPECT(sess, !alloc.allocated(run));
                    KTEST_EXPECT(sess, !alloc.allocated(run + 0x1000));
                });

            sess.end_suite();
        }

        inline void run_utility_suite(ktest::Session& sess) {
            sess.begin_suite("utility");

            run_case(sess, "byte-converter", [&]() {
                    using namespace kstd::ByteConverter;

                    KTEST_EXPECT(sess, convert(2048.0f, Unit::B, Unit::KB) == 2.0f);
                    KTEST_EXPECT(sess, convert(1.0f, Unit::MB, Unit::KB) == 1024.0f);
                    KTEST_EXPECT(sess, bestUnit(512.0f) == Unit::B);
                    KTEST_EXPECT(sess, bestUnit(1536.0f) == Unit::KB);
                    KTEST_EXPECT(sess, bestUnit(8.0f * 1024.0f * 1024.0f) == Unit::MB);
                });

            run_case(sess, "align-helpers", [&]() {
                    KTEST_EXPECT(sess, mm::align_down(0x0000u, mm::PAGE_SIZE) == 0x0000u);
                    KTEST_EXPECT(sess, mm::align_down(0x1FFFu, mm::PAGE_SIZE) == 0x1000u);
                    KTEST_EXPECT(sess, mm::align_up(0x2000u, mm::PAGE_SIZE) == 0x2000u);
                    KTEST_EXPECT(sess, mm::align_up(0x2001u, mm::PAGE_SIZE) == 0x3000u);
                });

            run_case(sess, "core-stack-anchor", [&]() {
                    smp::BaseCoreStack* stack0 = new smp::BaseCoreStack;
                    smp::BaseCoreStack* stack1 = new smp::BaseCoreStack;

                    KTEST_ASSERT(sess, stack0 != nullptr);
                    KTEST_ASSERT(sess, stack1 != nullptr);

                    auto* fake_core = reinterpret_cast<smp::Core*>(0x12345000);
                    stack0->init_stack_anchor(7, fake_core);

                    KTEST_EXPECT(sess, stack0->anchor.magic == smp::BaseCoreStack::MAGIC);
                    KTEST_EXPECT(sess, stack0->anchor.stack_id == 7);
                    KTEST_EXPECT(sess, stack0->anchor.core == fake_core);
                    KTEST_EXPECT(sess, stack0->base() + CORE_STACK_SIZE == stack0->initial_sp());

                    auto* anchor = smp::BaseCoreStack::anchor_from_sp(stack0->initial_sp() - 4);
                    KTEST_EXPECT(sess, anchor == &stack0->anchor);

                    stack1->copy_anchor_from(*stack0);
                    KTEST_EXPECT(sess, stack1->anchor.magic == smp::BaseCoreStack::MAGIC);
                    KTEST_EXPECT(sess, stack1->anchor.stack_id == 7);
                    KTEST_EXPECT(sess, stack1->anchor.core == fake_core);

                    delete stack0;
                    delete stack1;
                });

            run_case(sess, "isr-wrapper-metadata", [&]() {
                    constexpr auto pf   = idt::get_isr_wrapper<14>();
                    constexpr auto pit  = idt::get_isr_wrapper<32>();
                    constexpr auto soft = idt::get_isr_wrapper<48>();
                    constexpr auto bp   = idt::get_isr_wrapper<3>();

                    KTEST_EXPECT(sess, pf.has_error_code);
                    KTEST_EXPECT(sess, pf.kind == idt::InterruptFrameKind::Base);
                    KTEST_EXPECT(sess, pit.kind == idt::InterruptFrameKind::ContextSwitch);
                    KTEST_EXPECT(sess, !pit.has_error_code);
                    KTEST_EXPECT(sess, soft.kind == idt::InterruptFrameKind::ContextSwitch);
                    KTEST_EXPECT(sess, bp.kind == idt::InterruptFrameKind::Base);
                    KTEST_EXPECT(sess, !bp.has_error_code);
                });

            sess.end_suite();
        }

        inline void run_mm_suite(ktest::Session& sess, Kernel& kernel) {
            sess.begin_suite("mm");

            run_case(sess, "pmm-accounting-and-reserved-ranges", [&]() {
                    const uint32_t reserved_end = low_boot_reserved_end(kernel);
                    const uint32_t free_before  = mm::pmm::free_memory();
                    const uint32_t used_before  = mm::pmm::used_memory();

                    const uint32_t frame = mm::pmm::alloc_frame();
                    KTEST_ASSERT(sess, frame != 0);

                    KTEST_EXPECT(sess, (frame % mm::PAGE_SIZE) == 0);
                    KTEST_EXPECT(sess, frame >= reserved_end);
                    KTEST_EXPECT(sess, mm::pmm::used_memory() == used_before + mm::PAGE_SIZE);
                    KTEST_EXPECT(sess, mm::pmm::free_memory() == free_before - mm::PAGE_SIZE);

                    mm::pmm::free_frame(frame);

                    KTEST_EXPECT(sess, mm::pmm::used_memory() == used_before);
                    KTEST_EXPECT(sess, mm::pmm::free_memory() == free_before);
                });

            run_case(sess, "vmm-lookup-no-side-effects", [&]() {
                    const uint32_t virt = find_fresh_test_page_base();

                    KTEST_EXPECT(sess, !mm::pde_entry(virt).has_flag(mm::Present));
                    KTEST_EXPECT(sess, !mm::vmm::is_mapped(virt));
                    KTEST_EXPECT(sess, mm::vmm::virt_to_phys(virt) == 0xFFFFFFFFu);
                    KTEST_EXPECT(sess, !mm::pde_entry(virt).has_flag(mm::Present));
                });

            run_case(sess, "vmm-map-unmap", [&]() {
                    const uint32_t virt = find_fresh_test_page_base();
                    const uint32_t phys = mm::pmm::alloc_frame();
                    KTEST_ASSERT(sess, phys != 0);

                    KTEST_EXPECT(sess, !mm::vmm::is_mapped(virt));

                    mm::vmm::map_page(virt, phys, mm::Present | mm::Writable);

                    KTEST_EXPECT(sess, mm::pde_entry(virt).has_flag(mm::Present));
                    KTEST_EXPECT(sess, mm::vmm::is_mapped(virt));
                    KTEST_EXPECT(sess, mm::vmm::virt_to_phys(virt) == phys);
                    KTEST_EXPECT(sess, !mm::vmm::is_mapped(virt + mm::PAGE_SIZE));
                    KTEST_EXPECT(sess, mm::vmm::virt_to_phys(virt + mm::PAGE_SIZE) == 0xFFFFFFFFu);

                    volatile uint32_t* word = reinterpret_cast<volatile uint32_t*>(virt + 0x40);
                    *word                   = 0xA5A55A5A;
                    KTEST_EXPECT(sess, *word == 0xA5A55A5A);

                    mm::vmm::unmap_page(virt);

                    KTEST_EXPECT(sess, !mm::vmm::is_mapped(virt));
                    KTEST_EXPECT(sess, mm::vmm::virt_to_phys(virt) == 0xFFFFFFFFu);
                });

            run_case(sess, "heap-reuse-and-alignment", [&]() {
                    Heap& heap = kernel._heap.get();

                    void* reused0 = heap.alloc(96);
                    KTEST_ASSERT(sess, reused0 != nullptr);
                    memset(reinterpret_cast<uint8_t*>(reused0), 0x3C, 96);
                    KTEST_EXPECT(sess, reinterpret_cast<uint8_t*>(reused0)[0] == 0x3C);
                    KTEST_EXPECT(sess, reinterpret_cast<uint8_t*>(reused0)[95] == 0x3C);
                    heap.free(reused0);

                    void* reused1 = heap.alloc(96);
                    KTEST_ASSERT(sess, reused1 != nullptr);
                    KTEST_EXPECT(sess, reused1 == reused0);
                    heap.free(reused1);

                    void* aligned = heap.palignedAlloc(128);
                    KTEST_ASSERT(sess, aligned != nullptr);
                    KTEST_EXPECT(sess, (reinterpret_cast<uint32_t>(aligned) % mm::PAGE_SIZE) == 0);
                    memset(reinterpret_cast<uint8_t*>(aligned), 0xA7, 128);
                    KTEST_EXPECT(sess, reinterpret_cast<uint8_t*>(aligned)[0] == 0xA7);
                    KTEST_EXPECT(sess, reinterpret_cast<uint8_t*>(aligned)[127] == 0xA7);
                    heap.free(aligned);
                });

            run_case(sess, "heap-coalesce", [&]() {
                    Heap& heap = kernel._heap.get();

                    void* block0 = heap.alloc(0x200);
                    void* block1 = heap.alloc(0x200);
                    void* block2 = heap.alloc(0x200);

                    KTEST_ASSERT(sess, block0 != nullptr);
                    KTEST_ASSERT(sess, block1 != nullptr);
                    KTEST_ASSERT(sess, block2 != nullptr);

                    heap.free(block0);
                    heap.free(block1);

                    void* merged = heap.alloc(0x300);
                    KTEST_ASSERT(sess, merged != nullptr);
                    KTEST_EXPECT(sess, merged == block0);

                    heap.free(merged);
                    heap.free(block2);
                });

            sess.end_suite();
        }

        inline void run_scheduler_suite(ktest::Session& sess, Kernel& kernel) {
            sess.begin_suite("scheduler");

            run_case(sess, "task-execution", [&]() {
                    state().task1_runs     = 0;
                    state().task2_runs     = 0;
                    state().task1_finished = false;
                    state().task2_finished = false;

                    kernel._sched.get().create_task("ktest-task1", task1_entry);
                    kernel._sched.get().create_task("ktest-task2", task2_entry);

                    pit::sleep_us(250 * 1000);

                    KTEST_EXPECT(sess, state().task1_finished);
                    KTEST_EXPECT(sess, state().task2_finished);
                    KTEST_EXPECT(sess, state().task1_runs >= 2);
                    KTEST_EXPECT(sess, state().task2_runs >= 1);
                });

            sess.end_suite();
        }

        inline void run_all(Kernel& kernel) {
            if (state().ran_immediate)
                return;

            state().ran_immediate = true;

            auto& sess = ktest::session();
            sess.start("kernel self-tests");

            run_runtime_suite(sess);
            run_container_suite(sess);
            run_mm_suite(sess, kernel);
            run_utility_suite(sess);
            run_scheduler_suite(sess, kernel);

            LOG_INFO("[ktest] Immediate checks complete, waiting for shutdown checks\n");
        }

        inline void finalize() {
            if (!state().ran_immediate || state().ran_finalize)
                return;

            state().ran_finalize = true;

            auto& sess = ktest::session();
            sess.begin_suite("shutdown");

            run_case(sess, "atexit-callback", [&]() {
                    if (state().atexit_registered)
                        KTEST_EXPECT(sess, state().atexit_calls == 1);
                    else
                        KTEST_EXPECT(sess, state().atexit_calls == 0);
                });

            run_case(sess, "global-destructor", [&]() {
                    KTEST_EXPECT(sess, state().global_dtor_count == 1);
                });

            sess.end_suite();
            sess.complete();

            if (sess.failure_count)
                kstd::panic("kernel self-tests failed");
        }
    }
}
