/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_FILE_METER_H
#define REC_FILE_METER_H
#include "rec-observer.h"
#include <stddef.h>
#define SR_METER_FRAMES 4096
typedef struct {
    double peak_dbfs[2],rms_dbfs[2];
    uint64_t fullscale[2],frames,end_frame;
    unsigned rate;
} sr_file_meter;
/* Signed little-endian interleaved stereo PCM16/24 only; silence is -INFINITY.
 * Fullscale counts do not establish clipping upstream. */
int sr_meter_pcm(const unsigned char *pcm,size_t frames,unsigned bits,sr_file_meter *out);
/* Worker only, independent ordinary-path read. Reads the last <=4096 physically
 * written frames from a fresh observed file. Does not read /proc/fd or alter
 * recorder offset. Sparse sampled file windows are NOT a continuous clip log. */
int sr_meter_file(const sr_observation *observed,sr_file_meter *out);
#endif
