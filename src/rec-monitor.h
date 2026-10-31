/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_MONITOR_H
#define REC_MONITOR_H
#include <stdatomic.h>
#include "azmod-mod.h"
#include "rec-telemetry.h"
#include "rec-display.h"
#include "rec-file-meter.h"
/* Static lifetime: SDK does not join threads or unload a mod. Never free this
 * object until worker exit is observed and no UI caller remains. Init once. */
typedef struct {
    const az_core_v1 *api;
    sr_telemetry telemetry;
    sr_telemetry last_wav;
    sr_file_meter meter; /* guarded by telemetry.mutex */
    uint64_t meter_ms;
    int meter_valid;
    _Atomic uint64_t demand_ms;
    _Atomic int quit,exited;
    int initialized,visible;
    uint64_t last_redraw_ms;
} sr_monitor;
/* From init only. No native hooks. Caller aborts with stop on later init failure. */
int sr_monitor_init(sr_monitor *m,const az_core_v1 *api,const char *mod_id);
void sr_monitor_stop(sr_monitor *m);
/* UI thread only. SDK has no leave callback: on_draw must call paint; tick
 * does NOT refresh demand. Leaving the page therefore expires IO demand. */
void sr_monitor_enter(sr_monitor *m);
void sr_monitor_paint(sr_monitor *m);
void sr_monitor_leave(sr_monitor *m);
void sr_monitor_tick(sr_monitor *m);
void sr_monitor_view(sr_monitor *m,uint64_t reserve,sr_display *out);
int sr_monitor_last(sr_monitor *m,sr_observation *out);
int sr_monitor_meter(sr_monitor *m,sr_file_meter *out);
int sr_monitor_file(sr_monitor *m,sr_observation *out);
#endif
