#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/elf.hpp>
#include <klibcpp/vector.hpp>
#include <sys/ld.hpp>
#include <log.hpp>

using namespace kstd::ELF32;

struct Module {
    using entry_t = __cdecl int (*)(void*);

    Linker::Layout* layout;
    entry_t         enter;
    entry_t         exit;
};

class ModuleManager {
    public:
        ModuleManager(const ModuleManager&)            = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;

        static ModuleManager* get() {
            static ModuleManager* instance = new ModuleManager();
            return instance;
        }

        bool registerModule(void* ptr) {
            Object* obj = Object::create(ptr);
            if (!obj) {
                LOG_WARN("[module] Invalid ELF object at 0x%08x\n", ptr);
                return false;
            }

            Linker::Layout* layout = Linker::get()->load(obj);

            if (!layout) {
                LOG_WARN("[module] Failed to link module at 0x%08x\n", ptr);
                return false;
            }

            Module::entry_t enter = reinterpret_cast<Module::entry_t>(layout->find_symbol("module_enter"));
            Module::entry_t exit  = reinterpret_cast<Module::entry_t>(layout->find_symbol("module_exit"));

            if (enter)
                enter((void*)0xdeaddead);

            modules.push_back(new Module { layout, enter, exit });
            return true;
        }

    private:
        kstd::vector<Module*> modules;

        ModuleManager() = default;
};
