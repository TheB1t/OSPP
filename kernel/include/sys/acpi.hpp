#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <mm/vmm.hpp>
#include <log.hpp>

class acpi {
    public:
        struct SDTHeader {
            union {
                char        signature[4];
                uint32_t    signature_num;
            };
            uint32_t    length;
            uint8_t     revision;
            uint8_t     checksum;
            char        OEMID[6];
            char        OEM_table_ID[8];
            uint32_t    OEM_revision;
            uint32_t    creator_ID;
            uint32_t    creator_revision;
        } __packed;

        struct RSDT : public SDTHeader {
            SDTHeader*  other_sdts[];
        } __packed;

        struct XSDT : public SDTHeader {
            uint64_t    other_sdts[];
        } __packed;

        struct RSDP1 {
            char        signature[8];
            uint8_t     checksum;
            char        OEMID[6];
            uint8_t     revision;
            uint32_t    rsdt_address;
        } __packed;

        struct RSDP2 : public RSDP1 {
            uint32_t    length;
            uint64_t    xsdt_address;
            uint8_t     extendedChecksum;
            uint8_t     reserved[3];
        } __packed;

        static void init() {
            rsdp = find_rsdp();
            if (!rsdp) {
                LOG_WARN("[acpi] RSDP not found\n");
                return;
            }

            LOG_INFO("[acpi] RSDP found at 0x%08x\n", (uint32_t)rsdp);
            if (!validate_checksum(rsdp, rsdp->revision == 0 ? sizeof(RSDP1) : sizeof(RSDP2))) {
                LOG_WARN("[acpi] RSDP checksum invalid\n");
                return;
            }

            if (rsdp->revision == 0) {
                rsdt = (SDTHeader*)rsdp->rsdt_address;
            } else {
                rsdt = (SDTHeader*)(uint32_t)rsdp->xsdt_address;
            }

            if (!rsdt) {
                LOG_WARN("[acpi] RSDT not found\n");
                return;
            }

            LOG_INFO("[acpi] RSDT found at 0x%08x\n", (uint32_t)rsdt);
            // Map only SDT header for read it
            mm::vmm::map_page((uint32_t)rsdt, (uint32_t)rsdt, mm::Flags::Present | mm::Flags::Writable);
            if (!validate_checksum(rsdt)) {
                LOG_WARN("[acpi] RSDT checksum invalid\n");
                mm::vmm::unmap_page((uint32_t)rsdt);
                return;
            }

            // Map rest of SDT data
            mm::vmm::map_pages((uint32_t)rsdt, (uint32_t)rsdt, rsdt->length / mm::PAGE_SIZE, mm::Flags::Present | mm::Flags::Writable);
        }

        template<typename T>
        static T* find_sdth(const char* _sig) {
            uint32_t signature;
            memcpy32(&signature, (uint32_t*)_sig, 1);

            if (!rsdp)
                return nullptr;

            if (rsdp->revision >= 2) {
                XSDT* root = (XSDT*)rsdt;
                size_t entry_count = (root->length - sizeof(SDTHeader)) / sizeof(uint64_t);

                for (size_t i = 0; i < entry_count; ++i) {
                    SDTHeader* header = (SDTHeader*)(uint32_t)root->other_sdts[i];
                    if (header->signature_num == signature && validate_checksum(header))
                        return (T*)header;
                }
            } else {
                RSDT* root = (RSDT*)rsdt;
                size_t entry_count = (root->length - sizeof(SDTHeader)) / sizeof(uint32_t);

                for (size_t i = 0; i < entry_count; ++i) {
                    SDTHeader* header = (SDTHeader*)root->other_sdts[i];
                    if (header->signature_num == signature && validate_checksum(header))
                        return (T*)header;
                }
            }

            return nullptr;
        }

    private:
        static RSDP2* rsdp;
        static SDTHeader* rsdt;

        static RSDP2* find_rsdp() {
            for (uint32_t ptr = 0xE0000; ptr < 0x100000; ptr += sizeof(uint32_t)) {
                RSDP2* rsdp = (RSDP2*)ptr;
                if (strncmp(rsdp->signature, (char*)"RSD PTR ", 8) == 0)
                    return rsdp;
            }

            return nullptr;
        }

        template<typename T>
        static bool validate_checksum(T* data) {
            return validate_checksum(data, sizeof(T));
        }

        static bool validate_checksum(SDTHeader* hdr) {
            return validate_checksum(hdr, hdr->length);
        }

        static bool validate_checksum(void* data, size_t length) {
            uint8_t sum = 0;
            for (size_t i = 0; i < length; ++i)
                sum += ((uint8_t*)data)[i];

            return sum == 0;
        }
};