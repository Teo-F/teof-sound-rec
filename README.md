# Sound / Recording for AZ-MODS

**Experimental source preview — NOT device-validated, NOT an installation-ready release.**

An independently developed Community Mod candidate for the XDJ-AZ running AZ-MODS. Intended scope: **16-bit or24-bit stereo WAV recording at44.1kHz only**. The menu selection is intended for the next normal recording start, not an in-progress file. No other bit depths, codecs, or sample rates are exposed by this source.

This repository contains original mod code, an original icon, tests and GPL-3.0 license text. It does not contain vendor firmware, dumps, extracted firmware code, music, user libraries, SSH keys, authentication data or device logs. Not affiliated with AlphaTheta, Pioneer DJ or the AZ-MODS maintainer.

## Status and risks

- Local Linux AArch64 compilation and assembly unit tests have passed.
- The recorder hook has **not** been loaded or tested inside a real EP147 process.
- Recording start/stop/finalization, auto-splits, storage failures, concurrent playback and recovery remain unverified on hardware.
- The earlier generic host lifecycle model is not used by this hook; no claim of actual idle-state detection is made.
- A crash in a Community Mod can stop EP147 and audio playback. A successful package validation is not a runtime safety guarantee.
- Do not install on a performance device, during a set, or with a valuable recording in progress.
-24-bit output does not establish24-bit source fidelity, restore bandwidth or undo upstream clipping.24-bit stereo44100Hz PCM requires264600bytes/s, versus176400bytes/s at16-bit, excluding overhead.

No downloadable binary or repository-index entry is published at this stage. Do not copy this source directory into MODS.

## Exact compatibility boundary

SDK ABI1; intended minimum core1.2.5. Candidate firmware tag: `ep147-9e9454e357389ece`. Reference on-disk EP147 SHA256: `6577f42532375936c7e3fa830efa35b7daa9cde8e0a265c1fc7143f824321925`.

The hook checks firmware tag and16-byte code-region fingerprints before claiming recorder-start address0x230DAB0. Code fingerprints are not cryptographic authentication and do not validate every dependency. An unsupported build or a modified hook prefix must be refused, not bypassed.

The assembly wrapper only substitutes the incoming bits field for the established normal-start return address0xD611B8, stereo44100Hz, and incoming/requested16/24-bit values. Other callers and formats pass unchanged to the original trampoline. It preserves argument registers x0–x8, SP and incoming LR, avoiding a guessed C++ aggregate-return ABI. The menu has no live recorder-format readback. The caller/ownership and writer/split assumptions still need device validation.

## Build locally

Obtain the public [AZ-MODS SDK](https://github.com/Kyle-Hosman/az-mods-sdk) separately. Its header and pinned Linux arm64 Docker build environment are required. Do not substitute the Mac compiler for the final Linux shared library.

Build the SDK Dockerfile as `teof-az-sdk-local:review`, mount this directory read-only at `/mod`, the SDK include directory at `/sdk/include`, and an empty local build directory at `/build`. Inside that container:

```sh
make -f Makefile.experimental
gcc -std=c11 -O2 -Wall -Wextra -Werror test-hook.c test-hook-shim.S rec-start-aarch64.S -o /build/test-hook
/build/test-hook
```

Only `az_mod_v1_entry` may be dynamically exported. `Makefile.experimental` applies `exports.map` for this reason. The Docker build is not the SDK packer's image-label attestation.

`test-experimental-init.c` additionally tests tag/code/API refusal and registration order using code-prefix bytes read locally from a user-supplied reference EP147. It must run in an isolated disposable container. The firmware fixture is **not distributed here**; verify its entire SHA256 first. This test does not execute the firmware. Build it with `-D_GNU_SOURCE -I/sdk/include` and pass the reference file path as its only argument.

The SDK's default services-only sanitizer harness expects firmware tag `any`; it cannot by itself validate this exact-tag recorder hook. Its checks must not be reported as passed for this candidate.

## Before any device test

1. Validate the complete package against the SDK and check its binary hash.
2. Confirm exact active core/build compatibility and absence of competing hooks.
3. Keep a verified backup of USB MODS configuration and a documented offline disable route.
4. Finish all playback/recording before reload or power-cycle. Never unplug an actively used USB.
5. Start with a disposable16-bit test, stop/finalize and inspect the WAV; then test24-bit and switching back to16-bit.
6. Validate low-bit packing, splits and errors separately. Never label an unverified header-only result as preserved precision.

## Disable/recovery requirements

An installed hook cannot be removed from a running EP147 process merely by changing the16/24 selector. Disable the mod in Community Mods, then restart/reload only after all recordings are finalized and playback stopped. For failed UI startup, power down normally where possible, connect the test USB to the Mac and disable `community-mods.teof-sound-rec` in MODS/azmods.cfg, preserving other keys, before the next boot with that same USB. Verify the loader reports it off and that a subsequent baseline recording returns to16-bit. This route has been reviewed against retained loader source but **not tested with this mod on hardware**; no guaranteed recovery is claimed. Booting without the USB is not treated as a sufficient proof that staged cached code was removed.

Do not reflash firmware, delete unrelated settings, force-kill EP147 during recording, or mark a failed experiment safe merely because the device boots.

## License

GPL-3.0-only. The SDK is an external GPL-3.0 dependency; retain its license obligations when building or redistributing. Original Sound/REC icon assets are supplied under this repository's license.
