#pragma once

#include <klibcpp/bitmap.hpp>
#include <klibcpp/memory.hpp>
#include <klibcpp/static_array.hpp>
#include <klibcpp/static_slot.hpp>
#include <klibcpp/trivial.hpp>
#include <klibcpp/type_traits.hpp>
#include <klibcpp/utility.hpp>

#include <int/idt.hpp>
#include <mm/heap.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <sys/smp.hpp>

namespace ktest {
    namespace compile_time {
        using TinyBitmap = FixedBitmapAllocator<0x1000, 64, 0x1000>;
        using Seq4       = kstd::make_index_sequence<4>;

        template<typename T>
        T&& declval() noexcept;

        template<bool B>
        using enabled_u32 = kstd::enable_if_t<B, uint32_t>;

        struct BaseType {};
        struct DerivedType : BaseType {};
        struct UnrelatedType {};

        static_assert(kstd::is_same_v<Seq4, kstd::index_sequence<0, 1, 2, 3>>);

        static_assert(kstd::is_same_v<kstd::remove_reference<int&>::type, int>);
        static_assert(kstd::is_same_v<kstd::remove_reference<int&&>::type, int>);
        static_assert(kstd::is_pointer_v<int*>);
        static_assert(!kstd::is_pointer_v<int>);
        static_assert(kstd::is_reference_v<int&>);
        static_assert(kstd::is_rvalue_reference_v<int&&>);
        static_assert(kstd::is_enum_v<idt::InterruptFrameKind>);
        static_assert(kstd::is_same_v<enabled_u32<true>, uint32_t>);

        static_assert(kstd::is_base_of_v<BaseType, DerivedType>);
        static_assert(!kstd::is_base_of_v<DerivedType, BaseType>);
        static_assert(!kstd::is_base_of_v<BaseType, UnrelatedType>);

        static_assert(kstd::is_base_of_v<NonTransferable, TinyBitmap>);
        static_assert(kstd::is_base_of_v<NonTransferable, kstd::StaticSlot<int>>);
        static_assert(kstd::is_base_of_v<NonTransferable, kstd::StaticArray<int, 4>>);

        static_assert(TinyBitmap::BASE_ADDR == 0x1000);
        static_assert(TinyBitmap::MAX_UNITS == 64);
        static_assert(TinyBitmap::UNIT_SIZE == 0x1000);
        static_assert(TinyBitmap::BITMAP_SIZE == 2);
        static_assert(mm::PMM::BASE_ADDR == 0);
        static_assert(mm::PMM::MAX_UNITS == mm::MAX_FRAMES);
        static_assert(mm::PMM::UNIT_SIZE == mm::PAGE_SIZE);

        static_assert(mm::PAGE_SIZE == 4096);
        static_assert(mm::MAX_FRAMES == (1u << 20));
        static_assert(mm::align_down(mm::PAGE_SIZE, mm::PAGE_SIZE) == mm::PAGE_SIZE);
        static_assert(mm::align_down(0x1234u, 0x1000u) == 0x1000u);
        static_assert(mm::align_up(mm::PAGE_SIZE, mm::PAGE_SIZE) == mm::PAGE_SIZE);
        static_assert(mm::align_up(0x1234u, 0x1000u) == 0x2000u);
        static_assert(sizeof(mm::AlignedResult) == 8);
        static_assert(mm::PDE_BASE == 0xFFFFF000);
        static_assert(mm::PT_BASE == 0xFFC00000);
        static_assert(mm::PAGE_MASK == 0xFFFFF000);
        static_assert(sizeof(mm::Entry) == 4);
        static_assert(sizeof(HeapChunk_t) == 16);
        static_assert((HEAP_MIN_SIZE % mm::PAGE_SIZE) == 0);
        static_assert(HEAP_MIN_SIZE >= mm::PAGE_SIZE);

        static_assert(sizeof(idt::Entry) == 8);
        static_assert(sizeof(idt::Ptr) == 6);
        static_assert(sizeof(idt::BaseInterruptFrame) == 20);
        static_assert(sizeof(idt::InterruptFrame) == 76);
        static_assert(__builtin_offsetof(idt::InterruptFrame, base) == 56);
        static_assert(idt::get_isr_wrapper<14>().has_error_code);
        static_assert(idt::get_isr_wrapper<14>().kind == idt::InterruptFrameKind::Base);
        static_assert(idt::get_isr_wrapper<32>().kind == idt::InterruptFrameKind::ContextSwitch);
        static_assert(idt::get_isr_wrapper<48>().kind == idt::InterruptFrameKind::ContextSwitch);
        static_assert(!idt::get_isr_wrapper<3>().has_error_code);

        static_assert(sizeof(smp::StackAnchor) == 16);
        static_assert(sizeof(smp::BaseCoreStack) == CORE_STACK_SIZE);
        static_assert(alignof(smp::BaseCoreStack) == CORE_STACK_SIZE);
        static_assert(smp::BaseCoreStack::MAGIC == 0x434F5245);
        static_assert(smp::BaseCoreStack::kSize == CORE_STACK_SIZE);

        static_assert(kstd::StaticArray<int, 4>::npos == static_cast<size_t>(-1));
        static_assert(kstd::is_same_v<decltype(kstd::move(declval<int&>())), int&&>);
        static_assert(kstd::is_same_v<decltype(kstd::forward<int>(declval<int&>())), int&&>);
        static_assert(kstd::is_same_v<decltype(kstd::forward<int&>(declval<int&>())), int&>);
    }
}
