/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_METER_GRAPH_H
#define REC_METER_GRAPH_H
#include "azmod-mod.h"
#include "rec-file-meter.h"
#include <math.h>
#include <stdio.h>
#include <inttypes.h>
static inline int sr_db_position(double db,int width) {
    if(width<=0 || isnan(db) || db<=-60)return 0;
    if(db>=0)return width;
    return (int)((db+60)*width/60);
}
/* Pure drawing, provided copied meter only. Colored bar=window sample peak;
 * white underline=window RMS. No interpolation/peak hold across missing windows. */
static inline void sr_meter_graph(const az_core_v1 *api,az_draw *d,az_rect a,
                                  const sr_file_meter *m) {
    if(!api || !m || a.w<270 || a.h<190 || !AZ_HAS(api,fill_rect) ||
       !AZ_HAS(api,draw_text) || !AZ_HAS(api,color))return;
    uint32_t fg=api->color(AZ_COL_TEXT),dim=api->color(AZ_COL_TEXT_DIM);
    api->draw_text(d,a.x,a.y,"Written WAV levels (sampled)",14,api->color(AZ_COL_HI));
    int width=a.w-12,barh=20;
    for(int c=0;c<2;c++) {
        int y=a.y+28+c*58;
        char label[96];
        snprintf(label,sizeof(label),"%c   Peak %.1f dBFS    RMS %.1f dBFS",
                 c ? 'R':'L',m->peak_dbfs[c],m->rms_dbfs[c]);
        api->draw_text(d,a.x,y,label,12,fg);
        y+=21;
        api->fill_rect(d,a.x,y,width,barh,0x333333);
        int peak=sr_db_position(m->peak_dbfs[c],width);
        int rms=sr_db_position(m->rms_dbfs[c],width);
        const double edges[]={-60,-6,-1,0};
        const uint32_t colors[]={0x37b76b,0xeab332,0xed4545};
        for(int i=0;i<3;i++) {
            int start=sr_db_position(edges[i],width);
            int end=sr_db_position(edges[i+1],width);
            if(end>peak)end=peak;
            if(end>start)api->fill_rect(d,a.x+start,y,end-start,barh,colors[i]);
        }
        if(rms>0)api->fill_rect(d,a.x,y+barh-3,rms,3,fg);
        for(int x=7;x<width;x+=8)api->fill_rect(d,a.x+x,y,1,barh,0x1b2023);
    }
    const int ticks[]={-60,-36,-18,-6,0};
    for(int i=0;i<5;i++) {
        char label[12];snprintf(label,sizeof(label),"%d",ticks[i]);
        int x=a.x+sr_db_position(ticks[i],width);
        if(i==4)x-=16;
        api->draw_text(d,x,a.y+146,label,10,dim);
    }
    char note[120];
    snprintf(note,sizeof(note),"Fullscale in window L:%"PRIu64" R:%"PRIu64" | RMS underline",
             m->fullscale[0],m->fullscale[1]);
    api->draw_text(d,a.x,a.y+164,note,10,fg);
    if(a.h>=210)api->draw_text(d,a.x,a.y+184,
        "Storage delay; gaps between windows; not true peak",10,dim);
}
#endif
