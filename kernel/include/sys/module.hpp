#pragma once

#include "sys/smp.hpp"
#include <klibcpp/cstdint.hpp>
#include <klibcpp/spinlock.hpp>
#include <klibcpp/elf.hpp>
#include <klibcpp/static_array.hpp>
#include <klibcpp/static_slot.hpp>
#include <klibcpp/trivial.hpp>
#include <sys/ld.hpp>
#include <log.hpp>

using namespace kstd::ELF32;

struct Module {
    using entry_t = __cdecl int (*)(void*);

    Module(Linker::Layout* layout, entry_t enter, entry_t exit)
        : layout(layout), enter(enter), exit(exit) {};

    Linker::Layout* layout;
    entry_t         enter;
    entry_t         exit;
};

class ModuleManager : public NonTransferable {
    public:
        ModuleManager(Linker* linker)
            : _linker(linker) {};

        ~ModuleManager() {
            kstd::InterruptSpinLockGuard guard(lock_);

            for (auto& module : modules) {
                if (reinterpret_cast<uint32_t>(module.exit) >= 0x1000)
                    module.exit((void*)0xdeaddead);

                if (module.layout)
                    _linker->unload(module.layout);
            }

            modules.clear();
        }

        bool registerModule(void* ptr) {
            kstd::InterruptSpinLockGuard guard(lock_);

            Object* obj = Object::create(ptr);
            if (!obj) {
                LOG_WARN("[module] Invalid ELF object at 0x%08x\n", ptr);
                return false;
            }

            auto* layout = _linker->load(obj);

            if (!layout) {
                LOG_WARN("[module] Failed to link module at 0x%08x\n", ptr);
                return false;
            }

            Module::entry_t enter = reinterpret_cast<Module::entry_t>(layout->find_symbol("module_enter"));
            Module::entry_t exit  = reinterpret_cast<Module::entry_t>(layout->find_symbol("module_exit"));

            if (reinterpret_cast<uint32_t>(enter) < 0x1000) {
                LOG_ERR("[module] bad enter pointer: 0x%08x\n", reinterpret_cast<uint32_t>(enter));
                _linker->unload(layout);
                return false;
            }

            if (exit && reinterpret_cast<uint32_t>(exit) < 0x1000) {
                LOG_ERR("[module] bad exit pointer: 0x%08x\n", reinterpret_cast<uint32_t>(exit));
                _linker->unload(layout);
                return false;
            }

            if (enter) {
                LOG_INFO("[module] module_enter returned %d\n", enter((void*)0xdeaddead));
            }

            modules.emplace(layout, enter, exit);
            return true;
        }

    private:
        kstd::SpinLock lock_;
        Linker* _linker;
        kstd::StaticArray<Module, 64> modules;
};
