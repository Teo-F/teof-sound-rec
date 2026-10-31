/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_PIPELINE_H
#define REC_PIPELINE_H
#include "rec-engine.h"
typedef int (*sr_sink)(void *context, const float *l, const float *r, size_t frames);
typedef struct {
    sr_session session;
    sr_sink sink;
    void *context;
    int failed;
    unsigned part;
    float left[SR_MAX_BLOCK], right[SR_MAX_BLOCK];
} sr_pipeline;
/* All calls serialized by the owning writer worker. Sink consumes buffers
 * synchronously; it must never retain their pointers. No EP147 ABI assumed. */
int sr_pipeline_start(sr_pipeline *p, sr_format actual, sr_processing processing,
                      sr_sink sink, void *context);
int sr_pipeline_write(sr_pipeline *p, const float *l, const float *r, size_t n);
int sr_pipeline_split(sr_pipeline *p, sr_sink sink, void *context);
void sr_pipeline_stop(sr_pipeline *p);
#endif
