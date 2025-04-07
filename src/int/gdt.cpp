#include <int/gdt.hpp>

namespace gdt {
    void init_bsp() {
        serial::printf("[gdt] Initializing BSP's GDT. Entries %d\n", bsp_gdt_entries);
        init_core<true>(0);
        gdt_flush(reinterpret_cast<void*>(&bsp_ptr));
    }
}