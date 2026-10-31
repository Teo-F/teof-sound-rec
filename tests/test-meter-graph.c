/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-meter-graph.h"
#include <assert.h>
static int fills,texts;
static void fill(az_draw *d,int x,int y,int w,int h,uint32_t c) {
    (void)d;(void)c;assert(x>=10 && y>=20 && w>0 && h>0 && x+w<=550 && y+h<=240);fills++;
}
static void text(az_draw *d,int x,int y,const char *s,int px,uint32_t c) {
    (void)d;(void)s;(void)c;assert(x>=10 && x<550 && y>=20 && y+px*3/2<=240);texts++;
}
static uint32_t color(int role) {return (uint32_t)role;}
int main(void) {
    assert(sr_db_position(-INFINITY,100)==0 && sr_db_position(NAN,100)==0);
    assert(sr_db_position(-30,100)==50 && sr_db_position(5,100)==100);
    az_core_v1 api={.size=sizeof(api),.fill_rect=fill,.draw_text=text,.color=color};
    sr_file_meter meter={.peak_dbfs={-2.4,-5},.rms_dbfs={-15.3,-18.4}};
    sr_meter_graph(&api,0,(az_rect){10,20,540,220},&meter);
    assert(fills>=6 && texts>=10);
    fills=texts=0;
    sr_meter_graph(&api,0,(az_rect){10,20,100,40},&meter);assert(!fills && !texts);
    puts("meter graph: dB scaling, stereo bars, RMS underline and drawing bounds passed");
}
