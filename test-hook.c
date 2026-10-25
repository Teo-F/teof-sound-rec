#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

_Atomic uint32_t sound_rec_requested_bits = 16;
extern void test_original(void);
void *sound_rec_original_start = (void *)test_original;
extern void invoke_test_hook(void *info, uint64_t caller, uint64_t *registers);

static void check(int rate, int channels, int bits, int wanted, uint64_t caller, int result)
{
    uint32_t info[36];
    uint32_t expected[36];
    uint64_t registers[10] = {0};
    memset(info, 0x5a, sizeof info);
    info[2] = rate;
    info[3] = bits;
    info[4] = channels;
    memcpy(expected, info, sizeof info);
    expected[3] = result;
    atomic_store(&sound_rec_requested_bits, wanted);
    invoke_test_hook(info, caller, registers);
    assert(memcmp(info, expected, sizeof info) == 0);
    for (int index = 0; index < 9; index++)
        assert(registers[index] == (index == 2 ? (uint64_t)(uintptr_t)info : (uint64_t)(0x10 + index)));
    assert(registers[9] == caller);
}

int main(void)
{
    check(44100, 2, 16, 24, 0xd611b8, 24);
    check(44100, 2, 24, 16, 0xd611b8, 16);
    check(44100, 2, 16, 32, 0xd611b8, 16);
    check(44100, 2, 32, 24, 0xd611b8, 32);
    check(48000, 2, 16, 24, 0xd611b8, 16);
    check(96000, 2, 16, 24, 0xd611b8, 16);
    check(44100, 1, 16, 24, 0xd611b8, 16);
    check(44100, 2, 16, 24, 0xd611bc, 16);
    check(44100, 2, 16, 24, 0, 16);
    uint64_t registers[10] = {0};
    invoke_test_hook(NULL, 0xd611b8, registers);
    assert(registers[2] == 0);
    puts("PASS: actual AArch64 hook branches, exact bits-only write, x0-x8 and LR preservation, null/unrelated/unsupported pass-through");
    return 0;
}
