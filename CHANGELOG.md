# Changelog

## 0.9.2 — 2026-10-31 — compatibility correction

- Restore min_core 1.2.3 used by the working 0.9.0 package. The 0.9.1 manifest incorrectly confused the application version with the loader gate and was refused on the tested deck.
- Same executable SHA256 as tested 0.9.0; no audio or runtime guard changes.
- Fresh normal installer/Running confirmation remains pending.

## 0.9.1 — 2026-10-31 — public beta

- Publish the executable tested on the device as 0.9.0, with author **Teo-F**, current documentation and catalog metadata. No audio-code change from that tested binary.
- Add live REC headroom 0/−3/−6/−9/−12 dB, next-REC Auto Gain and linked −1 dBFS sample-peak limiter, initially Off.
- Keep 16/24-bit stereo PCM WAV at 44.1 kHz; add large native touch controls and sampled file-derived L/R meters, format/time/space and copied active gain telemetry.
- Use scoped private audio planes and generation-owned state; preserve original bypass and native write-chunk behavior, and retire state at complete worker destruction.
- Document measured tone, music, step/silence, longer-session, 16-bit and normal Disable/Reload/restore tests on one XDJ-AZ.
- Require AZ-MODS 1.2.7+ and exact firmware tag `ep147-9e9454e357389ece`.
- Explicitly retain beta limitations: native splits/storage errors/power loss/true-peak/timing/MASTER isolation are not hardware-validated. A 16-bit encoded peak can differ from the exact floating ceiling by less than one PCM step.

## 0.3.2 and earlier

Previous catalog release exposed the next-REC 16/24-bit choice. Previous downloads and legacy source history remain available.
