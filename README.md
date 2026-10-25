# Sound / Recording

![Sound Recording icon](icon.png)

An independent AZ-MODS Community Mod for XDJ-AZ by TEO-F. Choose **16-bit or24-bit stereo PCM WAV at44.1 kHz** from a settings page inside Community Mods. The highlighted button is the preference for the next recording. Default:16-bit.

**Public beta0.3.1.** Two short real-device recordings have been checked; long recordings, automatic splits and storage-error recovery remain unverified. Use a separate test USB and check your files before relying on this for a performance.

## Install

1. In AZ-MODS, open Community Mods > Repositories and add `Teo-F/teof-sound-rec`.
2. Refresh Browse, find Sound / Recording (with the blue sliders/orange recording icon), and install it on your test USB.
3. On the deck, enable Community Mods and this mod. Approve this build if prompted. Reload only after playback and recording have stopped.
4. Open Community Mods > Installed > Sound / Recording. Its settings appear in the right-hand pane, like Set Clock.
5. Tap16-bit or24-bit. With the browse knob, turn left/right to focus a button, then press to commit. The highlighted button shows your choice. Start and stop recording normally.

Changing the preference does not rewrite an existing recording. The page does not read back the currently open writer's format. Inspect a completed WAV to verify it. Version0.3.1 has the same executable as the device-tested0.3.0; only publication metadata/catalog changed. It does not migrate the ambiguous checkbox preference from0.2.x; its new setting defaults to16-bit.

## Supported build

- SDK ABI1, embedded core API minimum1.2.3.
- Exact EP147 tag: `ep147-9e9454e357389ece`.
- Refuses a different tag or a changed hook-prefix fingerprint.
- Records at44.1 kHz only. No48/96 kHz,32-bit or other codecs are exposed.

The title bar's displayed AZ-MODS version can differ from the embedded API version. A firmware update may make this mod refuse to load. Do not bypass its guards.

## What has been verified

On25October2026, the0.3.0 settings page was used on one XDJ-AZ. Completed files were read without alteration:

| Mode | Length | Result |
|---|---:|---|
|24-bit |17.048s | PCM24, stereo44100Hz, valid RIFF/data sizes and complete frames |
|16-bit |12.076s | PCM16, stereo44100Hz, valid RIFF/data sizes and complete frames |

The24-bit recording contained nonzero low bytes in1,168,300 of1,503,648 samples, excluding simple16-bit zero-padding. This is not proof of24 effective bits, bit-perfect source fidelity, or audible improvement. It cannot recover information already lost upstream.

Local page tests cover touch, knob focus/click, Back, invalid choices, unavailable state and stale/small layouts, with clang ASan/UBSan. The unchanged recorder wrapper previously passed192 synthetic adapter cases and exception-unwind tests in Linux AArch64. The0.3 page binary was cross-compiled using GCC15.2, exports only `az_mod_v1_entry`, and requires GLIBC2.17. No claim that a general services-only SDK harness validates this exact-tag hook.

Unverified: long sessions, automatic splits, storage-full/disconnect failures, power-loss recovery, every playback configuration, and reliable recovery from a crashing hook. A mod crash can interrupt audio.24-bit WAV consumes264600bytes/s, approximately50% more audio storage than16-bit.

## Disable / restore

Stop and finalize recording, then stop playback. Disable Sound / Recording under Community Mods > Installed and Reload once. Selecting16-bit alone leaves the hook installed. To remove the mod, use Remove from USB while idle, then reload.

If the UI cannot start, shut down normally where possible. Connect the test USB to a computer, back up MODS, set only `community-mods.teof-sound-rec = off` in MODS/azmods.cfg and create an empty `MODS/community-mods/teof-sound-rec/teof-sound-rec.off`. Return the same USB and use the normal boot flow. Confirm the mod is disabled and verify a fresh baseline recording. This is the loader-supported disable route; a full crash-recovery scenario has not been demonstrated for this release. Do not erase unrelated settings or flash firmware.

## Source and build

Active files: sound-rec.c, sound-page.h, recinfo-hook.S, exports.map, Makefile. The original0.1 source files retained in repository history/root (`experimental-mod.c`, `rec-start-aarch64.S`, `Makefile.experimental` and old tests) are superseded and are not used by this release.

Obtain the public [AZ-MODS SDK](https://github.com/Kyle-Hosman/az-mods-sdk) separately. Use its Linux AArch64 environment, or a Linux-targeting AArch64 cross compiler. `CC` can be supplied on the make command line:

```sh
make all SDK_INCLUDE=/path/to/az-mods-sdk/include OUT=/absolute/build/path CC=aarch64-unknown-linux-gnu-gcc
# On an AArch64 Linux test host/container:
make test OUT=/absolute/build/path
# Native page tests, macOS or Linux:
cc -std=c11 -O2 -Wall -Wextra -Werror -I/path/to/az-mods-sdk/include test-page.c -o /tmp/test-page
/tmp/test-page
```

Copy the built shared library into the mod directory before packaging with SDK tools/azmod_pack.py. Do not use a macOS-target library on the deck. The distributed binary and source match; package SHA256 and size are in azmods-repo.json.

Only original mod code/artwork, tests, GPL license and build outputs are distributed. No vendor firmware, manuals, music, device dumps or private configuration are included. Not affiliated with AlphaTheta, Pioneer DJ or the AZ-MODS maintainer. GPL-3.0-only; the original icon is under the same license.
