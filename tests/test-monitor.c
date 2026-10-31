/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "rec-monitor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static pthread_t worker;
static _Atomic int scans;
static _Atomic int stopped;
static int redraws,fail_start;
static void pause_ms(unsigned ms) {
    struct timespec t={ms/1000,(long)(ms%1000)*1000000}; nanosleep(&t,0);
}
int sr_observer_scan(sr_observation *o) {
    atomic_fetch_add(&scans,1);
    *o=(sr_observation){0};
    strcpy(o->path,"/media/usb/sda1/ALPHATHETA REC/REC001.tmp");
    o->wav.rate=44100; o->wav.bits=24; o->wav.channels=2;
    o->wav.device=o->storage.device=1; o->wav.inode=o->storage.inode=10;
    return atomic_load(&stopped) ? 0 : 1;
}
int sr_lastwav_check(const sr_observation *p,int result,sr_observation *out) {
    assert(result==0);*out=*p;out->wav.finalized_structure=1;
    strcpy(out->path,"/media/usb/sda1/ALPHATHETA REC/REC001.WAV");return 1;
}
int sr_meter_file(const sr_observation *o,sr_file_meter *out) {
    assert(o->wav.bits==24);
    *out=(sr_file_meter){.rate=44100,.frames=4096,.end_frame=(uint64_t)atomic_load(&scans)*4096};
    return 1;
}
static int start(const char *id,const char *name,void *(*fn)(void *),void *arg) {
    assert(!strcmp(id,"test-rec")); assert(!strcmp(name,"rec-file-watch"));
    if(fail_start) return -1;
    return pthread_create(&worker,0,fn,arg);
}
static void redraw(void) { redraws++; }
int main(void) {
    az_core_v1 api={.size=sizeof(api),.thread_start=start,.request_redraw=redraw};
    sr_monitor m={0},failed={0}; sr_display view;
    fail_start=1; assert(sr_monitor_init(&failed,&api,"test-rec"));
    assert(!failed.initialized); fail_start=0;
    assert(!sr_monitor_init(&m,&api,"test-rec"));
    pause_ms(150); assert(!atomic_load(&scans));
    sr_monitor_enter(&m); sr_monitor_tick(&m); assert(redraws);
    for(unsigned i=0;i<100 && !atomic_load(&scans);i++) pause_ms(10);
    /* Publication follows scan; poll snapshots rather than assume scheduling. */
    for(unsigned i=0;i<100;i++) {
        sr_monitor_view(&m,0,&view);
        if(strstr(view.format,"24-bit")) break;
        pause_ms(10);
    }
    assert(strstr(view.format,"24-bit"));
    sr_file_meter meter;
    for(unsigned i=0;i<100;i++) {
        if(sr_monitor_meter(&m,&meter))break;
        pause_ms(10);
    }
    assert(meter.frames==4096 && meter.rate==44100);
    atomic_store(&stopped,1);
    sr_observation last={0};
    for(unsigned i=0;i<150;i++) {
        sr_monitor_paint(&m);
        if(sr_monitor_last(&m,&last))break;
        pause_ms(10);
    }
    assert(last.wav.finalized_structure && strstr(last.path,"REC001.WAV"));
    /* UI snapshots must not wait on the worker publication mutex. */
    assert(!pthread_mutex_lock(&m.telemetry.mutex));
    sr_monitor_view(&m,0,&view); assert(strstr(view.format,"unavailable"));
    assert(!sr_monitor_meter(&m,&meter));
    assert(!pthread_mutex_unlock(&m.telemetry.mutex));
    /* No UI heartbeat: worker becomes dormant even if leave was not called. */
    pause_ms(1400); int count=atomic_load(&scans);
    pause_ms(600); assert(atomic_load(&scans)==count);
    sr_monitor_view(&m,0,&view); assert(strstr(view.format,"unavailable"));
    sr_monitor_paint(&m); sr_monitor_tick(&m);
    for(unsigned i=0;i<100 && atomic_load(&scans)==count;i++) pause_ms(10);
    assert(atomic_load(&scans)>count);
    for(unsigned i=0;i<100;i++) {
        if(sr_monitor_last(&m,&last))break;
        pause_ms(10);
    }
    assert(last.wav.finalized_structure); /* revalidated after menu pause */
    sr_monitor_leave(&m); sr_monitor_view(&m,0,&view);
    assert(strstr(view.format,"unavailable"));
    sr_monitor_stop(&m); assert(!pthread_join(worker,0));
    assert(atomic_load(&m.exited));
    sr_telemetry_destroy(&m.telemetry);
    sr_telemetry_destroy(&m.last_wav);
    puts("monitor: dormant start, UI demand, heartbeat pause/resume, nonblocking view, stop and start failure passed");
}
