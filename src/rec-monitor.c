/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "rec-monitor.h"
#include "rec-lastwav.h"
#include <time.h>
#include <errno.h>
static uint64_t mono_ms(void) {
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)) return 0;
    return (uint64_t)t.tv_sec*1000+(unsigned long)t.tv_nsec/1000000;
}
static void *monitor_worker(void *arg) {
    sr_monitor *m=arg;
    uint64_t last_scan=0;
    int was_demanded=0;
    sr_observation previous={0};
    int have_previous=0;
    uint64_t previous_seen=0;
    int final_seen=0;
    uint64_t meter_end=0,meter_dev=0,meter_ino=0;
    while(!atomic_load_explicit(&m->quit,memory_order_acquire)) {
        uint64_t now=mono_ms();
        uint64_t demand=atomic_load_explicit(&m->demand_ms,memory_order_acquire);
        int demanded=now && demand && now>=demand && now-demand<=1000;
        if(demanded && (!was_demanded || now-last_scan>=125)) {
            sr_observation o;
            int result=sr_observer_scan(&o);
            /* Slow IO must not refresh data using its completion time. Reads
             * begun before a pause remain stale or cleared afterwards. */
            if(atomic_load_explicit(&m->demand_ms,memory_order_acquire)==0 ||
               atomic_load_explicit(&m->quit,memory_order_acquire)) result=0;
            sr_telemetry_publish(&m->telemetry,result,&o,now);
            sr_file_meter meter={0};
            int meter_ok=result==1 && sr_meter_file(&o,&meter);
            pthread_mutex_lock(&m->telemetry.mutex);
            if(meter_ok) {
                int changed=!m->meter_valid || meter_end!=meter.end_frame ||
                    meter_dev!=o.wav.device || meter_ino!=o.wav.inode;
                m->meter=meter;m->meter_valid=1;
                if(changed)m->meter_ms=now;
                meter_end=meter.end_frame;meter_dev=o.wav.device;meter_ino=o.wav.inode;
            } else m->meter_valid=0;
            pthread_mutex_unlock(&m->telemetry.mutex);
            sr_observation final={0};
            int found=0;
            if(result==1) {
                previous=o;have_previous=1;previous_seen=now;final_seen=0;
            } else if(result==0 && have_previous) {
                /* Retain a pending observed identity across short menu visits.
                 * A previously verified final file is always revalidated on
                 * return, never displayed from an old cached result alone. */
                if(final_seen || (now>=previous_seen && now-previous_seen<=60000)) {
                    found=sr_lastwav_check(&previous,result,&final);
                    if(found)final_seen=1;
                } else have_previous=0;
            } else if(result!=0) {have_previous=0;final_seen=0;}
            sr_telemetry_publish(&m->last_wav,found,&final,now);
            last_scan=now;
        } else if(!demanded && was_demanded) {
            sr_telemetry_publish(&m->telemetry,0,0,now);
            sr_telemetry_publish(&m->last_wav,0,0,now);
            pthread_mutex_lock(&m->telemetry.mutex);
            m->meter_valid=0;
            pthread_mutex_unlock(&m->telemetry.mutex);
            /* Snapshots expire; candidate identity remains for bounded retry.
             * Process restart still discards all history. */
        }
        was_demanded=demanded;
        struct timespec pause={0,25000000};
        while(nanosleep(&pause,&pause) && errno==EINTR) {
            if(atomic_load_explicit(&m->quit,memory_order_acquire)) break;
        }
    }
    sr_telemetry_publish(&m->telemetry,0,0,mono_ms());
    sr_telemetry_publish(&m->last_wav,0,0,mono_ms());
    atomic_store_explicit(&m->exited,1,memory_order_release);
    return 0;
}
int sr_monitor_init(sr_monitor *m,const az_core_v1 *api,const char *id) {
    if(!m || m->initialized || !api || !id ||
       !AZ_HAS(api,thread_start) || !AZ_HAS(api,request_redraw)) return -1;
    if(sr_telemetry_init(&m->telemetry)) return -1;
    if(sr_telemetry_init(&m->last_wav)) {
        sr_telemetry_destroy(&m->telemetry);return -1;
    }
    m->api=api; m->visible=0; m->last_redraw_ms=0;
    atomic_init(&m->demand_ms,0); atomic_init(&m->quit,0); atomic_init(&m->exited,0);
    m->initialized=1;
    if(api->thread_start(id,"rec-file-watch",monitor_worker,m)) {
        m->initialized=0;
        sr_telemetry_destroy(&m->telemetry);
        sr_telemetry_destroy(&m->last_wav);
        return -1;
    }
    return 0;
}
void sr_monitor_stop(sr_monitor *m) {
    if(!m || !m->initialized) return;
    m->visible=0;
    atomic_store_explicit(&m->demand_ms,0,memory_order_release);
    atomic_store_explicit(&m->quit,1,memory_order_release);
}
void sr_monitor_enter(sr_monitor *m) {
    if(!m || !m->initialized) return;
    m->visible=1;
    sr_monitor_paint(m);
}
void sr_monitor_paint(sr_monitor *m) {
    if(!m || !m->initialized ||
       atomic_load_explicit(&m->quit,memory_order_acquire)) return;
    m->visible=1;
    atomic_store_explicit(&m->demand_ms,mono_ms(),memory_order_release);
}
void sr_monitor_leave(sr_monitor *m) {
    if(!m || !m->initialized) return;
    m->visible=0;
    atomic_store_explicit(&m->demand_ms,0,memory_order_release);
}
void sr_monitor_tick(sr_monitor *m) {
    if(!m || !m->initialized || !m->visible ||
       atomic_load_explicit(&m->quit,memory_order_acquire)) return;
    uint64_t now=mono_ms();
    uint64_t demand=atomic_load_explicit(&m->demand_ms,memory_order_acquire);
    if(!now || !demand || now<demand || now-demand>1000) return;
    if(now-m->last_redraw_ms>=250) {
        m->last_redraw_ms=now;
        m->api->request_redraw();
    }
}
void sr_monitor_view(sr_monitor *m,uint64_t reserve,sr_display *out) {
    sr_observation o;
    int valid=m && m->initialized && m->visible &&
        !atomic_load_explicit(&m->quit,memory_order_acquire) &&
        sr_telemetry_read(&m->telemetry,mono_ms(),&o);
    sr_display_build(valid,valid ? &o : 0,reserve,out);
}
int sr_monitor_last(sr_monitor *m,sr_observation *out) {
    if(!out)return 0;
    *out=(sr_observation){0};
    return m && m->initialized && m->visible &&
        !atomic_load_explicit(&m->quit,memory_order_acquire) &&
        sr_telemetry_read(&m->last_wav,mono_ms(),out);
}
int sr_monitor_file(sr_monitor *m,sr_observation *out) {
    if(!out)return 0;
    *out=(sr_observation){0};
    return m && m->initialized && m->visible &&
        !atomic_load_explicit(&m->quit,memory_order_acquire) &&
        sr_telemetry_read(&m->telemetry,mono_ms(),out);
}
int sr_monitor_meter(sr_monitor *m,sr_file_meter *out) {
    if(!out)return 0;
    *out=(sr_file_meter){0};
    if(!m || !m->initialized || !m->visible ||
       atomic_load_explicit(&m->quit,memory_order_acquire))return 0;
    uint64_t now=mono_ms();
    if(pthread_mutex_trylock(&m->telemetry.mutex))return 0;
    int valid=m->meter_valid && now>=m->meter_ms && now-m->meter_ms<=1500;
    if(valid)*out=m->meter;
    pthread_mutex_unlock(&m->telemetry.mutex);
    return valid;
}
