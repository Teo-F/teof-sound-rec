/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_MONITOR_PANEL_H
#define REC_MONITOR_PANEL_H
#include "rec-monitor.h"
#include "rec-native-panel.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "rec-meter-graph.h"
/* Call from existing native screen on_draw with its allocated telemetry area.
 * on_enter calls sr_monitor_enter; mod tick calls sr_monitor_tick. Existing
 * next-bit controls remain caller-owned and separate from this passive panel. */
static inline void sr_monitor_panel_draw(sr_monitor *m,az_draw *d,az_rect area,
                                          uint64_t reserve_bytes) {
    if(!m || !m->initialized) return;
    sr_monitor_paint(m);
    sr_display view;
    sr_monitor_view(m,reserve_bytes,&view);
    sr_native_panel(m->api,d,area,&view);
}
static inline void sr_monitor_levels_draw(sr_monitor *m,az_draw *d,az_rect area) {
    sr_monitor_paint(m);
    sr_file_meter meter;
    sr_display v={0};
    snprintf(v.status,sizeof(v.status),"Written PCM levels: unavailable");
    snprintf(v.format,sizeof(v.format),"Start REC; wait for written PCM samples.");
    if(sr_monitor_meter(m,&meter)) {
        if(area.w>=270 && area.h>=190) {
            sr_meter_graph(m->api,d,area,&meter);return;
        }
        snprintf(v.status,sizeof(v.status),"Sampled WAV levels; storage delay");
        snprintf(v.format,sizeof(v.format),"Peak L: %.1f | R: %.1f dBFS",meter.peak_dbfs[0],meter.peak_dbfs[1]);
        snprintf(v.elapsed,sizeof(v.elapsed),"RMS L: %.1f | R: %.1f dBFS",meter.rms_dbfs[0],meter.rms_dbfs[1]);
        snprintf(v.space,sizeof(v.space),"Window fullscale samples L: %"PRIu64" | R: %"PRIu64,
                 meter.fullscale[0],meter.fullscale[1]);
        snprintf(v.remaining,sizeof(v.remaining),"Window: %.0f ms; not continuous clip detection",
                 meter.rate ? (double)meter.frames*1000/meter.rate : 0);
    }
    sr_native_panel(m->api,d,area,&v);
}
static inline void sr_monitor_last_draw(sr_monitor *m,az_draw *d,az_rect area) {
    if(!m || !m->initialized)return;
    sr_monitor_paint(m);
    sr_observation o;
    int found=sr_monitor_last(m,&o);
    sr_display v={0};
    snprintf(v.status,sizeof(v.status),"No verified last WAV observed");
    snprintf(v.format,sizeof(v.format),"Keep this page open during REC and Stop.");
    if(found) {
        const char *name=strrchr(o.path,'/');name=name ? name+1 : o.path;
        snprintf(v.status,sizeof(v.status),"File: %.80s",name);
        snprintf(v.format,sizeof(v.format),"PCM %u-bit / %u Hz / %u ch",
                 o.wav.bits,o.wav.rate,o.wav.channels);
        snprintf(v.elapsed,sizeof(v.elapsed),"Duration: %.3f seconds",o.wav.seconds);
        snprintf(v.space,sizeof(v.space),"RIFF/header/size: consistent");
        snprintf(v.remaining,sizeof(v.remaining),"Structure check only; audio not assessed.");
    }
    /* Drawing helper's generic heading remains 'Recording file'. */
    sr_native_panel(m->api,d,area,&v);
}
#endif
