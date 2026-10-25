#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>

extern "C" {
int run_adapter_tests(void);
extern void *sound_rec_original_info;
uintptr_t invoke_info_hook(void *info);
}

static int destroyed;
struct Cleanup {
    ~Cleanup() { destroyed++; }
};

static void throwing_original()
{
    Cleanup cleanup;
    throw 147;
}

int main()
{
    run_adapter_tests();
    sound_rec_original_info = reinterpret_cast<void *>(throwing_original);
    unsigned char output[192];
    std::memset(output, 0x5a, sizeof output);
    bool caught = false;
    try {
        invoke_info_hook(output);
    } catch (int value) {
        assert(value == 147);
        caught = true;
    }
    assert(caught && destroyed == 1);
    for (unsigned char byte : output) assert(byte == 0x5a);
    std::puts("PASS: C++ exception unwinds through adapter CFI, original cleanup runs, output not modified");
}
