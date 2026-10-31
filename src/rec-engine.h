/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_ENGINE_H
#define REC_ENGINE_H
#include <stddef.h>
#include <stdint.h>
#define SR_MAX_BLOCK 256
typedef struct {
    unsigned rate, channels, bits;
    int headroom_db; /* attenuation: 0,3,6,9,12 */
} sr_format;
typedef struct { int auto_gain, limiter; } sr_processing;
typedef struct {
    int active;
    sr_format format;
    uint64_t frames, clips;
    double peak_in, peak_out;
    sr_processing processing;
    double rms_power, agc_db, limiter_gain, reduction_db;
} sr_session;
/* Worker-owned objects; publish a copied snapshot to the UI under a separate
 * synchronization mechanism. These functions are not concurrent access APIs. */
int sr_format_valid(sr_format f);
int sr_start(sr_session *s, sr_format actual);
int sr_start_processing(sr_session *s, sr_format actual, sr_processing processing);
int sr_process(sr_session *s, const float *left, const float *right,
               float *out_left, float *out_right, size_t frames);
void sr_stop(sr_session *s);
double sr_elapsed(const sr_session *s);
uint64_t sr_space_seconds(sr_format actual, uint64_t available, uint64_t reserve);
int sr_prefix_valid(const char *prefix);
#endif
