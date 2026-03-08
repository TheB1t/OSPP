#pragma once

#include <klibcpp/static_slot.hpp>

#include <multiboot.hpp>
#include <mm/heap.hpp>
#include <sys/apic.hpp>
#include <sys/module.hpp>
#include <sys/smp.hpp>
#include <sched/scheduler.hpp>

class Kernel {
    public:
        kstd::StaticSlot<Heap> _heap;
        kstd::StaticSlot<Linker> _linker;
        kstd::StaticSlot<ModuleManager> _mmanager;
        kstd::StaticSlot<apic> _apic;
        kstd::StaticSlot<sched::Scheduler> _sched;
        kstd::StaticSlot<smp::CoreManager> _cmanager;

        multiboot_info_t _mboot;

        void init(multiboot_info_t* mboot);
        ~Kernel();
};
