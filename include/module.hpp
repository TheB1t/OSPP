#pragma once

#include <multiboot.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/vector.hpp>
#include <crc32.hpp>
#include <log.hpp>

namespace k {
    static constexpr uint32_t MAGIC = 0x4f535050;
    static constexpr uint8_t VERSION = 1;

    enum class ModuleType : uint8_t {
        DEBUG_SYMBOLS = 0,
    };

    struct ModuleHeader {
        uint32_t magic;
        char name[32];
        uint8_t version;
        ModuleType type;
        uint32_t data_size;
    } __packed;

    enum class LoadResult {
        SUCCESS,
        INVALID_MAGIC,
        UNSUPPORTED_VERSION,
        INVALID_CRC,
        INVALID_SIZE,
        ALLOCATION_FAILED,
    };

    class BaseKernelModule {
        public:
            char name[32];
            uint8_t* data;

            BaseKernelModule(char name[32], uint8_t* data) : data(data) {
                strncpy(this->name, name, 32);
            }

            virtual ModuleType type() const = 0;
    };

    class DebugSymbolsModule : public BaseKernelModule {
        public:
            struct Header {
                uint32_t size;
                uint32_t strtab_offset;
            } __packed;

            struct Symbol {
                uint32_t address;
                uint32_t name_offset;
            } __packed;

            Header* header;
            Symbol* symbols;
            char* strtab;

            DebugSymbolsModule(char name[32], uint8_t* data) 
                : BaseKernelModule(name, data),
                header(reinterpret_cast<Header*>(data)),
                symbols(reinterpret_cast<Symbol*>(data + sizeof(Header))),
                strtab(reinterpret_cast<char*>(data + header->strtab_offset)) {}

            Symbol* nearest_symbol(uint32_t address) {
                Symbol* nearest = nullptr;
                uint32_t nearest_distance = 0xffffffff;

                for (uint32_t i = 0; i < header->size; i++) {
                    uint32_t distance = address - symbols[i].address;
                    if (distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = &symbols[i];
                    }
                }

                if (nearest_distance > 0x1000)
                    return nullptr;

                return nearest;
            }

            const char* lookup_symbol(uint32_t address) {
                Symbol* symbol = nearest_symbol(address);
                return symbol ? &strtab[symbol->name_offset] : nullptr;
            }

            ModuleType type() const override {
                return ModuleType::DEBUG_SYMBOLS;
            }
    };

    class KernelModuleRegistry {
        public:
            static KernelModuleRegistry* get() {
                static KernelModuleRegistry instance;
                return &instance;
            }

            BaseKernelModule* create_module(ModuleHeader* header, uint8_t* data_start) {
                switch (header->type) {
                    case ModuleType::DEBUG_SYMBOLS:
                        return new DebugSymbolsModule(header->name, data_start);
                    default:
                        return nullptr;
                }
            }

            void load_modules(multiboot_info_t* mboot) {
                if (!mboot->mods_count) {
                    LOG_WARN("No modules found\n");
                    return;
                }

                multiboot_module_t* mods = (multiboot_module_t*)mboot->mods_addr;

                for (uint32_t i = 0; i < mboot->mods_count; i++) {
                    multiboot_module_t* mod = &mods[i];
                    uint8_t* data_start = (uint8_t*)mod->mod_start;
                    uint8_t* data_end = (uint8_t*)mod->mod_end;
                    uint32_t data_size = data_end - data_start;

                    load(data_start, data_size);
                }
            }

            LoadResult load(void* data, uint32_t size) {
                if (size < sizeof(ModuleHeader) + sizeof(uint32_t)) {
                    return LoadResult::INVALID_SIZE;
                }

                ModuleHeader* header = static_cast<ModuleHeader*>(data);

                if (header->magic != MAGIC) {
                    LOG_WARN("Module magic number is invalid\n");
                    return LoadResult::INVALID_MAGIC;
                }

                if (header->version != VERSION) {
                    LOG_WARN("Module version is not supported\n");
                    return LoadResult::UNSUPPORTED_VERSION;
                }

                const uint32_t expected_size = sizeof(ModuleHeader) + header->data_size + sizeof(uint32_t);

                if (size < expected_size) {
                    LOG_WARN("Buffer too small for module (needed %u, got %u)\n", expected_size, size);
                    return LoadResult::INVALID_SIZE;
                }

                uint8_t* data_start = reinterpret_cast<uint8_t*>(data) + sizeof(ModuleHeader);
                uint32_t expected_crc = *reinterpret_cast<uint32_t*>(data_start + header->data_size);
                uint32_t actual_crc = crc32::calc(data, header->data_size + sizeof(ModuleHeader), 0xFFFFFFFF);

                if (actual_crc != expected_crc) {
                    LOG_WARN("Module CRC32 checksum is invalid, expected 0x%08x, got 0x%08x\n", expected_crc, actual_crc);
                    return LoadResult::INVALID_CRC;
                }

                LOG_INFO("Module loaded: %s\n", header->name);

                BaseKernelModule* mod = create_module(header, data_start);
                if (!mod)
                    return LoadResult::ALLOCATION_FAILED;

                modules.push_back(mod);
                return LoadResult::SUCCESS;
            }

            template<typename T>
            T* by_name(const char* name) {
                for (uint32_t i = 0; i < modules.size(); i++) {
                    if (!modules[i])
                        continue;

                    if (strcmp(modules[i]->name, (char*)name) == 0) {
                        return static_cast<T*>(modules[i]);
                    }
                }

                return nullptr;
            }

            DebugSymbolsModule* debug_module() {
                return by_name<DebugSymbolsModule>("debug");
            }

            KernelModuleRegistry(const KernelModuleRegistry&) = delete;
            KernelModuleRegistry& operator=(const KernelModuleRegistry&) = delete;

        private:
            kstd::vector<BaseKernelModule*> modules;

            KernelModuleRegistry() = default;
    };
}