#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include "azmod-mod.h"

#define MOD_ID "teof-sound-rec"
#define FW_TAG "ep147-9e9454e357389ece"

_Atomic uint32_t sound_rec_requested_bits = 16;
void *sound_rec_original_start;
extern void sound_rec_start_hook(void);

static const az_core_v1 *services;
static const char *const tags[] = {FW_TAG, NULL};
static const char *const choices[] = {"16-bit", "24-bit"};
static uint64_t code_fingerprint(uintptr_t address)
{
    const unsigned char *bytes = (const unsigned char *)address;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned index = 0; index < 16; index++)
        hash = (hash ^ bytes[index]) * UINT64_C(1099511628211);
    return hash;
}

static void preference_changed(int value, void *user)
{
    (void)user;
    if (value == 0 || value == 1)
        atomic_store_explicit(&sound_rec_requested_bits, value == 1 ? 24 : 16, memory_order_release);
}

static int initialize(const az_core_v1 *api)
{
    if (!api || api->abi != AZ_MOD_ABI || !AZ_HAS(api, fw_tag) ||
        !AZ_HAS(api, claim_hook) || !AZ_HAS(api, register_row) || !AZ_HAS(api, row_value)) return -1;
    const char *tag = api->fw_tag();
    if (!tag || strcmp(tag, FW_TAG) != 0) return -2;
    if (code_fingerprint(0x230DAB0) != UINT64_C(0x7cbd263e2da90a63) ||
        code_fingerprint(0xD61160) != UINT64_C(0x9a9506dcd4bf3b9c)) return -3;
    services = api;
    const az_row_v1 group = {
        .size = sizeof(az_row_v1), .key = "sound", .label = "Sound / Recording (TEST)",
        .kind = AZ_ROW_GROUP,
        .help = "Experimental recorder control. 44.1 kHz only. Not a native96 unlock.",
    };
    if (services->register_row(MOD_ID, &group) < 0) return -4;
    const az_row_v1 bits = {
        .size = sizeof(az_row_v1), .key = "rec-bits", .label = "Next REC bit depth",
        .parent = "sound", .kind = AZ_ROW_CHOICE, .choices = choices, .n_choices = 2,
        .default_choice = 0, .on_change = preference_changed,
        .help = "16/24-bit for the next normal stereo WAV recording at44.1 kHz. Changes do not alter an existing recording. No source-fidelity guarantee.",
    };
    int handle = services->register_row(MOD_ID, &bits);
    if (handle < 0) return -5;
    int saved = services->row_value(handle);
    if (saved != 0 && saved != 1) return -6;
    preference_changed(saved, NULL);
    if (services->claim_hook(MOD_ID, 0x230DAB0, (void *)sound_rec_start_hook,
                            &sound_rec_original_start) != 0) return -7;
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
