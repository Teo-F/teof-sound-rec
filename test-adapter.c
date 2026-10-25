#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Atomic uint32_t sound_rec_requested_bits = 16;
extern void original_fixture(void);
void *sound_rec_original_info = (void *)original_fixture;
extern uintptr_t invoke_info_hook(void *info);
int original_calls;
uint32_t fixture_fields[3];

int run_adapter_tests(void)
{
    const int rates[] = {44100, 48000, 96000, 0};
    const int channels[] = {1, 2, 0};
    const int depths[] = {8, 16, 24, 32};
    const int selections[] = {16, 24, 32, 0};
    int count = 0;
    for (unsigned selected = 0; selected < 4; selected++)
        for (unsigned rate = 0; rate < 4; rate++)
            for (unsigned channel = 0; channel < 3; channel++)
                for (unsigned depth = 0; depth < 4; depth++) {
                    _Alignas(16) unsigned char output[192];
                    unsigned char expected[192];
                    memset(output, 0x5a, sizeof output);
                    memcpy(expected, output, sizeof output);
                    fixture_fields[0] = rates[rate];
                    fixture_fields[1] = depths[depth];
                    fixture_fields[2] = channels[channel];
                    memcpy(expected + 8, fixture_fields, sizeof fixture_fields);
                    int eligible = rates[rate] == 44100 && channels[channel] == 2 &&
                        (depths[depth] == 16 || depths[depth] == 24) &&
                        (selections[selected] == 16 || selections[selected] == 24);
                    uint32_t expected_bits = eligible ? selections[selected] : depths[depth];
                    memcpy(expected + 12, &expected_bits, sizeof expected_bits);
                    atomic_store_explicit(&sound_rec_requested_bits, selections[selected], memory_order_release);
                    int before = original_calls;
                    assert(invoke_info_hook(output) == UINT64_C(0xaabbccdd));
                    assert(original_calls == before + 1);
                    assert(memcmp(output, expected, sizeof output) == 0);
                    count++;
                }
    int before = original_calls;
    assert(invoke_info_hook(NULL) == UINT64_C(0xaabbccdd));
    assert(original_calls == before + 1);
    printf("PASS: %d adapter cases, hidden x8 output retained after original clobber, x0-x7 arguments, x0 return, x19 preservation, exactly one original call, exact output bytes, null forwarding\n", count);
    return 0;
}
