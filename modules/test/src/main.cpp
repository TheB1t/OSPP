#include <log.hpp>

__extern_c {
    __cdecl int module_enter(void* ptr __unused) {
        LOG_INFO("Welcome from module_enter\n");
        return 500;
    }

    __cdecl int module_exit(void* ptr __unused) {
        LOG_INFO("Welcome from module_exit\n");
        return 0;
    }
}