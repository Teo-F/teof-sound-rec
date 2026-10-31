/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_TELEMETRY_H
#define REC_TELEMETRY_H
#include <pthread.h>
#include "rec-observer.h"
/* Own worker publishes copies; UI never receives a native pointer or file fd.
 * Caller provides monotonic milliseconds from the same clock domain. */
typedef struct {
    pthread_mutex_t mutex;
    sr_observation observation;
    uint64_t stamp_ms;
    int available;
} sr_telemetry;
int sr_telemetry_init(sr_telemetry *t);
void sr_telemetry_destroy(sr_telemetry *t); /* only after worker/UI have stopped */
void sr_telemetry_publish(sr_telemetry *t,int scan_result,
                          const sr_observation *observation,uint64_t now_ms);
/* 1=fresh unique file candidate, 0=unknown/stale. Not a native REC-state API. */
int sr_telemetry_read(sr_telemetry *t,uint64_t now_ms,sr_observation *out);
#endif
