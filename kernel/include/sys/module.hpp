#pragma once

#include "sys/smp.hpp"
#include <klibcpp/cstdint.hpp>
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

        bool registerModule(void* ptr) {
            Object* obj = Object::create(ptr);
            if (!obj) {
                LOG_WARN("[module] Invalid ELF object at 0x%08x\n", ptr);
                return false;
            }

            return false;

            _linker->load(obj);

            return false;

            // if (!layout) {
            //     LOG_WARN("[module] Failed to link module at 0x%08x\n", ptr);
            //     return false;
            // }

            // Module::entry_t enter = reinterpret_cast<Module::entry_t>(layout->find_symbol("module_enter"));
            // Module::entry_t exit  = reinterpret_cast<Module::entry_t>(layout->find_symbol("module_exit"));

            // LOG_INFO("[module] layout=%p enter=%p exit=%p\n", layout, enter, exit);

            // if (reinterpret_cast<uint32_t>(enter) < 0x1000) {
            //     LOG_ERR("[module] bad enter pointer: 0x%08x\n", reinterpret_cast<uint32_t>(enter));
            //     return false;
            // }

            // if (enter) {
            //     LOG_INFO("module_enter returned %d\n", enter((void*)0xdeaddead));
            // }

            // modules.emplace(layout, enter, exit);
            // return true;
        }

    private:
        Linker* _linker;
        kstd::StaticArray<Module, 64> modules;
};
