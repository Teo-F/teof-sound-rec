/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_NATIVE_PANEL_H
#define REC_NATIVE_PANEL_H
#include "azmod-mod.h"
#include "rec-display.h"
/* Draw inside a caller-owned native Community Mods area. Caller supplies a
 * snapshot copied outside this function. No registration, IO or REC controls. */
static inline void sr_native_panel(const az_core_v1 *api,az_draw *d,
                                   az_rect area,const sr_display *view) {
    if(!api || !view || !AZ_HAS(api,draw_text) || !AZ_HAS(api,text_width) ||
       !AZ_HAS(api,color) || !AZ_HAS(api,row_height)) return;
    int rh=area.h/6;
    if(rh>32)rh=32;
    if(rh<16)return;
    int px=rh/2; if(px>16)px=16;
    const char *lines[]={"Recording file",view->status,view->format,
                        view->elapsed,view->space,view->remaining};
    for(int i=0;i<6;i++) {
        if(area.w<100 || area.h<rh || i>(area.h-rh)/rh) break;
        int font=px;
        while(font>8 && api->text_width(lines[i],font)>area.w) font--;
        /* Avoid drawing outside narrow native panels when minimum font fails. */
        if(api->text_width(lines[i],font)>area.w) continue;
        api->draw_text(d,area.x,area.y+i*rh,lines[i],font,
                      api->color(i==0 ? AZ_COL_HI : AZ_COL_TEXT));
    }
}
#endif
