#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include "experimental-mod.c"

void sound_rec_start_hook(void) {}
static const char *reported_tag = FW_TAG;
static int row_count;
static int hook_calls;
static int stored_choice;
static int fail_registration = -1;
static int refuse_hook;
static az_row_v1 captured_bits;

static const char *firmware_tag(void) { return reported_tag; }

static int register_test_row(const char *id, const az_row_v1 *row)
{
    assert(strcmp(id, MOD_ID) == 0);
    if (row_count == fail_registration) return -1;
    if (row->kind == AZ_ROW_CHOICE) captured_bits = *row;
    return row_count++;
}

static int saved_row_value(int handle)
{
    assert(handle == 1);
    return stored_choice;
}

static int claim_test_hook(const char *id, uintptr_t address, void *hook, void **original)
{
    assert(strcmp(id, MOD_ID) == 0 && address == 0x230DAB0);
    assert(row_count == 2 && hook == (void *)sound_rec_start_hook);
    hook_calls++;
    if (refuse_hook) return -1;
    *original = (void *)sound_rec_start_hook;
    return 0;
}

static void map_signature(uintptr_t address, FILE *fixture)
{
    uintptr_t page = address & ~(uintptr_t)4095;
    void *mapped = mmap((void *)page, 4096, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(mapped != MAP_FAILED && mapped == (void *)page);
    assert(fseek(fixture, (long)(address - 0x400000), SEEK_SET) == 0);
    assert(fread((void *)address, 1, 16, fixture) == 16);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    FILE *fixture = fopen(argv[1], "rb");
    assert(fixture);
    az_core_v1 api = {.abi = AZ_MOD_ABI, .size = sizeof(az_core_v1),
        .fw_tag = firmware_tag, .claim_hook = claim_test_hook,
        .register_row = register_test_row, .row_value = saved_row_value};
    assert(initialize(NULL) == -1);
    az_core_v1 short_api = api;
    short_api.size = offsetof(az_core_v1, row_value);
    assert(initialize(&short_api) == -1);
    reported_tag = "ep147-0000000000000000";
    assert(initialize(&api) == -2 && hook_calls == 0);
    reported_tag = NULL;
    assert(initialize(&api) == -2 && hook_calls == 0);
    reported_tag = FW_TAG;
    map_signature(0x230DAB0, fixture);
    map_signature(0xD61160, fixture);
    assert(fclose(fixture) == 0);
    *(unsigned char *)(uintptr_t)0x230DAB0 ^= 1;
    assert(initialize(&api) == -3 && hook_calls == 0);
    *(unsigned char *)(uintptr_t)0x230DAB0 ^= 1;
    *(unsigned char *)(uintptr_t)0xD61160 ^= 1;
    assert(initialize(&api) == -3 && hook_calls == 0);
    *(unsigned char *)(uintptr_t)0xD61160 ^= 1;
    fail_registration = 0;
    assert(initialize(&api) == -4 && hook_calls == 0);
    fail_registration = 1;
    assert(initialize(&api) == -5 && hook_calls == 0);
    row_count = 0;
    fail_registration = -1;
    stored_choice = -1;
    assert(initialize(&api) == -6 && hook_calls == 0);
    row_count = 0;
    stored_choice = 0;
    refuse_hook = 1;
    assert(initialize(&api) == -7 && hook_calls == 1);
    row_count = 0;
    stored_choice = 1;
    refuse_hook = 0;
    assert(initialize(&api) == 0 && hook_calls == 2);
    assert(atomic_load(&sound_rec_requested_bits) == 24);
    captured_bits.on_change(0, NULL);
    assert(atomic_load(&sound_rec_requested_bits) == 16);
    captured_bits.on_change(32, NULL);
    assert(atomic_load(&sound_rec_requested_bits) == 16);
    assert(munmap((void *)(uintptr_t)0x230D000, 4096) == 0);
    assert(munmap((void *)(uintptr_t)0xD61000, 4096) == 0);
    puts("PASS: actual init with synthetic code pages, tag/signature/API guards, rows-before-hook ordering, saved choice and refusal paths");
    return 0;
}
