# Validation record — 31 October 2026

One XDJ-AZ, exact EP147 `ep147-9e9454e357389ece`, embedded AZ-MODS core 1.2.7. The distributed 0.9.1 executable is byte-identical to the locally tested 0.9.0. Recordings were started/stopped manually. File copies were read by ordinary path after finalization; SHA256 matched device and local copies. Recorded music and proprietary material are not published.

## Host tests

The supplied `make test` runs ASan/UBSan tests for gains, bypass, linked ceiling, block invariance, silence gate and gain bounds; storage/WAV/observer validation; coherent telemetry and nonblocking UI views; lifecycle generations, in-flight retirement and reused identities; fixed-capacity state and 10,000 concurrent begin/retire iterations against worker leases; private-plane processing; original argument/result and unwind forwarding; sink-failure latch/no retry; next-REC option coupling; and native page drawing/input bounds.

Local split/subblock tests preserve an envelope across a changed writer within the same worker. They do **not** reproduce an automatic native file split, real storage-full/disconnect or power loss. Injected sink failure proves local control flow, not native error recovery.

SDK package validation passes. GCC 15.2 AArch64 rebuild from the public source matches tested executable SHA256 `8d68dd5b26c72d660639b9e0a6c0fe793b12bbafa9bcda992701a08bd44e32d1`. The validator reports two extra compiler runtime initializers previously identified as libgcc LSE/SME setup, not mod audio setup.

## Hardware measurements

- **Headroom:** −6 dB vs 0 dB repeated 1 kHz source. Median steady-window RMS difference −6.00 dB in both channels. Host tests also cover 0/−3/−9/−12 coefficients, source preservation and native 2048-frame write chunks. Other presets lack this paired hardware measurement.
- **Limiter:** 128.747755 s, PCM24/44100 stereo music. Full-rate whole-file sample peaks −1.00000087 dBFS L/R, no sample above −1 dBFS or at full scale. Active telemetry showed limiter On, AGC Off and 0.6 dB reduction. This is sample-peak, not true-peak validation.
- **Steady Auto Gain:** 34.560363 s recording of nominal −18 dBFS sample-peak 1 kHz source. Initial RMS around −21.84 dBFS rose at about 0.4983 dB/s; steady median RMS −18.0007 dBFS L/R, reached around 8 s. No full-scale samples.
- **Step/silence:** 99.947755 s recording of a 95 s source: low −30 dBFS peak / high −3 / silence / low / high / silence. Rise about 0.4867 and 0.4845 dB/s, fall −2.9911 dB/s, high steady RMS −18.0007. Low plateau relative to extrapolated initial input supported approximately +6 dB correction. Middle 9.946 s was exactly zero PCM; telemetry and low-return fit supported holding gain over silence. This is digital silence, not an analog noise-floor measurement. Peaks stayed below −1 dBFS but this particular test did not force visible limiter reduction.
- **Longer session:** 2092.468027 s (34:52), PCM24/44100 stereo, consistent RIFF/data sizes. Full-file peaks −1.00000087 dBFS L/R; zero samples above −1 or at full scale. The owner reported normal operation and confirmed the 449.822 s silence at 16:43.146–24:12.968 was a pause after a track ended. About 27 minutes were outside the long zero intervals; this is not 30 minutes of continuous music stress.
- **16-bit:** 115.435465 s, PCM16/44100 stereo, consistent RIFF/data sizes, no full-scale endpoints. Peaks −1.286785 L / −0.999854 R dBFS. Two right samples encoded as −29205 exceed the exact −1 dBFS value by less than one PCM16 step; none exceed the threshold plus one step. Do not describe this as zero samples above the exact threshold.
- **Lifecycle and restore:** native UI counters after a take showed construction/destruction 1/1, missing 0, failure 0. Normal Disable/Reload left no own module mapping or own hook in the observed current table. Stock recorder then finalized a 10.819410 s PCM16/44100 WAV. Normal Enable/Reload restored 0.9.0 and all five hooks. This does not validate every crash/recovery scenario.

## Remaining limits

Hardware validation remains open for every preset/mode combination, automatic file splits, full/disconnected storage, power-loss/crashing-hook recovery, prolonged continuous music, repeated multi-hour takes, worst-case CPU/stack timing, independent MASTER-output isolation and true-peak/inter-sample behavior. A consistent header and endpoint checks do not prove no dropped samples without an input reference. No native 48/96 kHz or effective-bit-depth improvement is claimed.
