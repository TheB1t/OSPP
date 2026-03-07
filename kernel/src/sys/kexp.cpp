#include <sys/kexp.hpp>
#include <log.hpp>

namespace kexp {
    Entry* lookup(const char* name) {
        if (!name)
            return nullptr;

        uint8_t* start = (uint8_t*)&__kexp_start;
        uint8_t* end   = (uint8_t*)&__kexp_end;

        uint64_t nh    = kstd::hash_fold32_cstr(name);
        uint32_t n     = (uint32_t)(end - start) / sizeof(Entry);

        Entry*   tab   = (Entry*)start;

        for (uint32_t i = 0; i < n; ++i) {
            auto* e = &tab[i];
            if (e->name_hash == nh)
                return e;
        }

        return nullptr;
    }
}