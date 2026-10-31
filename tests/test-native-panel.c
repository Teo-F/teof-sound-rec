/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "sound-rec-unified.c"
void sound_rec_info_hook(void) {}
static int saved,texts,colors;
#ifdef SR_NATIVE_HEADROOM
static int headroom_saved;
#endif
#ifdef SR_NATIVE_DYNAMICS
static int dynamics_saved;
#endif
static az_rect bounds;
static void save(const char *id,const char *key,int value) {
    assert(!strcmp(id,MOD_ID));
    if(!strcmp(key,"record-bits"))saved=value;
#ifdef SR_NATIVE_DYNAMICS
    else if(!strcmp(key,"rec-dynamics-next"))dynamics_saved=value;
#endif
#ifdef SR_NATIVE_HEADROOM
    else {assert(!strcmp(key,"rec-headroom-db-live"));headroom_saved=value;}
#else
    else assert(0);
#endif
}
static void redraw(void) {}
static int width(const char *s,int px){return (int)strlen(s)*px/2;}
static uint32_t color(int role){return (uint32_t)role;}
static void text(az_draw *d,int x,int y,const char *s,int px,uint32_t c) {
    (void)d;(void)c;assert(x>=bounds.x && y>=bounds.y && x+width(s,px)<=bounds.x+bounds.w);
    assert(y+px*3/2<=bounds.y+bounds.h);texts++;
}
static void fill(az_draw *d,int x,int y,int w,int h,uint32_t c) {
    (void)d;assert(x>=bounds.x && y>=bounds.y && w>0 && h>0);
    assert(x+w<=bounds.x+bounds.w && y+h<=bounds.y+bounds.h);
    if(c==0x44df66 || c==0xffcc36 || c==0xf14444)colors++;
}
int main(void) {
    az_core_v1 api={.size=sizeof(api),.cfg_set=save,.request_redraw=redraw,
        .draw_text=text,.text_width=width,.color=color,.fill_rect=fill};
    C=&api;ready=1;
    assert(!sr_telemetry_init(&sound_monitor.telemetry));
    assert(!sr_telemetry_init(&sound_monitor.last_wav));
    sound_monitor.api=&api;sound_monitor.initialized=1;
    atomic_init(&sound_monitor.quit,0);atomic_init(&sound_monitor.demand_ms,0);
    unified_enter(0);
    bounds=(az_rect){710,390,540,300};unified_draw(0,bounds,0);assert(texts && native_hits);
    az_event ev={.kind=AZ_EV_TAP,.x=native_bits[0].x+2,.y=native_bits[0].y+2};
    assert(unified_input(&ev,0) && saved==24 && native_bits[0].h>=54);
    ev.x=native_bits[1].x+2;assert(unified_input(&ev,0) && saved==16);
    ev=(az_event){.kind=AZ_EV_TAP,.x=1100,.y=430};
#ifdef SR_NATIVE_DYNAMICS
    assert(unified_input(&ev,0) && sr_native_dynamics_get()==0 && dynamics_saved==0);
    ev=(az_event){.kind=AZ_EV_TAP,.x=native_dynamics[0][1].x+2,.y=native_dynamics[0][1].y+2};
    assert(unified_input(&ev,0) && sr_native_dynamics_get()==3 && dynamics_saved==3);
    ev=(az_event){.kind=AZ_EV_TAP,.x=native_dynamics[1][0].x+2,.y=native_dynamics[1][0].y+2};
    assert(unified_input(&ev,0) && sr_native_dynamics_get()==0 && dynamics_saved==0);
#else
    assert(!unified_input(&ev,0) && saved==16);
#endif
#ifdef SR_NATIVE_HEADROOM
    ev=(az_event){.kind=AZ_EV_TAP,.x=native_headroom[2].x+2,.y=native_headroom[2].y+2};
    assert(unified_input(&ev,0) && headroom_saved==6 && sr_native_headroom_get()==6);
#endif
    struct timespec now;assert(!clock_gettime(CLOCK_MONOTONIC,&now));
    sound_monitor.meter=(sr_file_meter){.rate=44100,.frames=4096,
        .peak_dbfs={-4.1,-4.3},.rms_dbfs={-15,-16}};
    sound_monitor.meter_valid=1;
    sound_monitor.meter_ms=(uint64_t)now.tv_sec*1000+(unsigned long)now.tv_nsec/1000000;
    unified_draw(0,bounds,0);assert(colors>0);
    bounds=(az_rect){0,0,430,260};unified_draw(0,bounds,0);
    bounds=(az_rect){0,0,120,40};unified_draw(0,bounds,0);assert(!native_hits);
    ev=(az_event){.kind=AZ_EV_TAP,.x=10,.y=10};assert(!unified_input(&ev,0));
    ev.kind=AZ_EV_BACK;assert(!unified_input(&ev,0));
    sr_telemetry_destroy(&sound_monitor.telemetry);sr_telemetry_destroy(&sound_monitor.last_wav);
    puts("native REC panel: reference layout, real bars,16/24, disabled controls, bounds and BACK passed");
}
