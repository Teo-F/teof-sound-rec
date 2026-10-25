#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include "azmod-mod.h"

#define MOD_ID "teof-sound-rec"
#define FW_TAG "ep147-9e9454e357389ece"
#define RECINFO_ADDRESS ((uintptr_t)0xD59BD0)

_Atomic uint32_t sound_rec_requested_bits = 16;
void *sound_rec_original_info;
extern void sound_rec_info_hook(void);

static const char *const tags[] = {FW_TAG, NULL};
static int ready;
#include "sound-page.h"

static uint64_t code_fingerprint(uintptr_t address)
{
    const unsigned char *bytes = (const unsigned char *)address;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned index = 0; index < 16; index++)
        hash = (hash ^ bytes[index]) * UINT64_C(1099511628211);
    return hash;
}

static int initialize(const az_core_v1 *api)
{
    ready = 0;
    if (!api || api->abi != AZ_MOD_ABI || !AZ_HAS(api, fw_tag) ||
        !AZ_HAS(api, claim_hook) || !page_api_ok(api)) return -1;
    const char *tag = api->fw_tag();
    if (!tag || strcmp(tag, FW_TAG) != 0) return -2;
    if (code_fingerprint(RECINFO_ADDRESS) != UINT64_C(0x86c42f21f1578379)) return -3;
    C = api;
    int saved = C->cfg_get(MOD_ID, "record-bits", 16);
    atomic_store_explicit(&sound_rec_requested_bits, saved == 24 ? 24 : 16, memory_order_release);
    if (C->register_screen(MOD_ID, &PAGE) != 0) return -4;
    if (api->claim_hook(MOD_ID, RECINFO_ADDRESS, (void *)sound_rec_info_hook,
                        &sound_rec_original_info) != 0) return -7;
    ready = 1;
    return 0;
}

static const az_mod_v1 mod = {
    .abi = AZ_MOD_ABI, .size = sizeof(az_mod_v1), .id = MOD_ID,
    .fw_tags = tags, .init = initialize,
};

AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void)
{
    return &mod;
}
