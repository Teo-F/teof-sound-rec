/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-meter-graph.h"
static const az_core_v1 *C;
static az_rect native_bits[2];
#ifdef SR_NATIVE_DYNAMICS
static az_rect native_dynamics[2][2];
static void choose_dynamics(int group,int on) {
    if(!ready)return;
    int flags=sr_native_dynamics_get();
    if(group==0)flags=on ? 3:(flags&2); /* AGC ON also selects limiter. */
    else flags=on ? (flags|2):0; /* Limiter OFF also turns AGC OFF. */
    if(sr_native_dynamics_set(flags))C->cfg_set(MOD_ID,"rec-dynamics-next",flags);
    C->request_redraw();
}
#endif
#ifdef SR_NATIVE_HEADROOM
static az_rect native_headroom[5];
static void choose_headroom(int db) {
    if(!ready || !sr_native_headroom_set(db))return;
    C->cfg_set(MOD_ID,"rec-headroom-db-live",db);C->request_redraw();
}
#endif
static int native_hits,selected;
static int page_api_ok(const az_core_v1 *api) {
    return AZ_HAS(api,cfg_get) && AZ_HAS(api,cfg_set) && AZ_HAS(api,register_screen) &&
        AZ_HAS(api,draw_text) && AZ_HAS(api,fill_rect) && AZ_HAS(api,color) &&
        AZ_HAS(api,text_width) && AZ_HAS(api,request_redraw);
}
static void choose_bits(int bits) {
    if(!ready || (bits!=16 && bits!=24))return;
    atomic_store_explicit(&sound_rec_requested_bits,bits,memory_order_release);
    C->cfg_set(MOD_ID,"record-bits",bits);C->request_redraw();
}
static void panel_text(az_draw *d,int x,int y,int width,const char *s,int px,uint32_t ink) {
    while(px>8 && C->text_width(s,px)>width)px--;
    if(C->text_width(s,px)<=width)C->draw_text(d,x,y,s,px,ink);
}
static void native_button(az_draw *d,az_rect r,const char *s,int lit,int enabled) {
    C->fill_rect(d,r.x,r.y,r.w,r.h,lit && enabled ? 0xff8c00 : 0x30363a);
    panel_text(d,r.x+5,r.y+(r.h-(r.h>=36 ? 21:16))/2,r.w-10,s,r.h>=36 ? 14:11,
               enabled ? (lit ? 0 : 0xffffff) : 0x92999e);
}
static void unified_draw(az_draw *d,az_rect a,void *user) {
    (void)user;native_hits=0;sr_monitor_paint(&sound_monitor);
    if(a.w<430 || a.h<260) {
        if(a.w>=120 && a.h>=20)panel_text(d,a.x,a.y,a.w,"REC panel: more space needed",10,C->color(AZ_COL_TEXT_DIM));
        return;
    }
    const uint32_t fg=0xffffff,dim=0xb3bbc1,bg=0x151b1f;
    az_rect settings=a;
    int controls=a.h-124;if(controls>180)controls=180;
    int top=a.h-controls-8;
    a.y+=controls+8;a.h=top; /* compact readings below the large controls */
    C->fill_rect(d,a.x,a.y,a.w,top,bg);
    sr_observation o;sr_display view;
    int file=sr_monitor_file(&sound_monitor,&o);
    sr_display_build(file,file ? &o:0,UINT64_C(64)*1024*1024,&view);
    panel_text(d,a.x+8,a.y+4,a.w-16,view.format,10,fg);
    
    char line[160];
    snprintf(line,sizeof(line),"%.55s | %.40s",view.space,view.elapsed);
    panel_text(d,a.x+8,a.y+22,a.w-16,line,9,dim);
    sr_file_meter meter;
    int measured=sr_monitor_meter(&sound_monitor,&meter);
    int step=22,barw=a.w-126;
    for(int c=0;c<2;c++) {
        int y=a.y+42+c*step;
        C->draw_text(d,a.x+8,y,c ? "R":"L",12,fg);
        int peak=measured ? sr_db_position(meter.peak_dbfs[c],barw):0;
        int rms=measured ? sr_db_position(meter.rms_dbfs[c],barw):0;
        int x=a.x+28;
        C->fill_rect(d,x,y,barw,14,0x30383c);
        for(int n=0;n<barw;n+=7) {
            int end=n+5;if(end>barw)end=barw;
            if(end>peak)end=peak;
            uint32_t color=n<sr_db_position(-6,barw) ? 0x44df66 :
                n<sr_db_position(-1,barw) ? 0xffcc36:0xf14444;
            if(end>n)C->fill_rect(d,x+n,y,end-n,14,color);
        }
        if(rms>0)C->fill_rect(d,x,y+13,rms,1,fg);
        if(measured)snprintf(line,sizeof(line),"%.1f dBFS",meter.peak_dbfs[c]);
        else snprintf(line,sizeof(line),"-- dBFS");
        panel_text(d,a.x+a.w-90,y,a.w>500 ? 84:80,line,11,fg);
    }
    int y=a.y+42+2*step;
    if(measured)snprintf(line,sizeof(line),"Sample peak %.1f dBFS | Window fullscale L:%"PRIu64" R:%"PRIu64,
        fmax(meter.peak_dbfs[0],meter.peak_dbfs[1]),meter.fullscale[0],meter.fullscale[1]);
    else snprintf(line,sizeof(line),"Sample peak unavailable | Window fullscale unavailable");
    panel_text(d,a.x+8,y,a.w-16,line,11,fg);
#ifdef SR_NATIVE_PROBE
    char probe[96];
#ifdef SR_NATIVE_DYNAMICS
    if(file)sr_native_dynamics_status(probe,sizeof(probe));
    else sr_native_probe_status(probe,sizeof(probe));
#else
    sr_native_probe_status(probe,sizeof(probe));
#endif
    snprintf(line,sizeof(line),"Sampled WAV / storage delay / gaps | %.95s",probe);
    panel_text(d,a.x+8,y+17,a.w-16,line,8,dim);
#else
    panel_text(d,a.x+8,y+17,a.w-16,"Sampled WAV / storage delay / gaps / RMS underline",8,dim);
#endif
    int cy=settings.y,lw=(a.w*56)/100,rw=a.w-lw-8,rx=a.x+lw+8;
    int row=(controls-6)/2,label=row>=60 ? 11:9,button_y=22;
    C->fill_rect(d,a.x,cy,lw,row,bg);C->fill_rect(d,rx,cy,rw,row,bg);
    panel_text(d,a.x+5,cy+2,lw-10,"Bit depth (next REC)",label,dim);
    unsigned bits=atomic_load_explicit(&sound_rec_requested_bits,memory_order_acquire);
    for(int i=0;i<2;i++) {
        native_bits[i]=(az_rect){a.x+5+i*((lw-10)/2),cy+button_y,(lw-14)/2,row-27};
        native_button(d,native_bits[i],i ? "16-bit":"24-bit",bits==(unsigned)(i ? 16:24),1);
    }
#ifdef SR_NATIVE_DYNAMICS
    int flags=sr_native_dynamics_get();
    panel_text(d,rx+5,cy+2,rw-10,"Auto Gain + Limiter (next REC)",label,dim);
    for(int i=0;i<2;i++) {
        az_rect b={rx+5+i*((rw-10)/2),cy+button_y,(rw-14)/2,row-27};
        native_dynamics[0][i]=b;native_button(d,b,i ? "ON":"OFF",!!(flags&1)==i,1);
    }
#else
    panel_text(d,rx+5,cy+2,rw-10,"REC Auto Gain - under development",label,dim);
    native_button(d,(az_rect){rx+5,cy+button_y,(rw-14)/2,row-27},"OFF",0,0);
    native_button(d,(az_rect){rx+rw/2+2,cy+button_y,(rw-14)/2,row-27},"ON",0,0);
#endif
    cy+=row+6;
    C->fill_rect(d,a.x,cy,lw,row,bg);C->fill_rect(d,rx,cy,rw,row,bg);
#ifdef SR_NATIVE_HEADROOM
    panel_text(d,a.x+5,cy+2,lw-10,"REC headroom (live attenuation)",label,dim);
#else
    panel_text(d,a.x+5,cy+2,lw-10,"Headroom - under development",label,dim);
#endif
    const char *levels[]={"0 dB","-3 dB","-6 dB","-9 dB","-12 dB"};
    for(int i=0;i<5;i++) {
        az_rect b={a.x+5+i*((lw-10)/5),cy+button_y,(lw-14)/5,row-27};
#ifdef SR_NATIVE_HEADROOM
        native_headroom[i]=b;native_button(d,b,levels[i],sr_native_headroom_get()==i*3,1);
#else
        native_button(d,b,levels[i],0,0);
#endif
    }
#ifdef SR_NATIVE_DYNAMICS
    panel_text(d,rx+5,cy+2,rw-10,"Limiter -1dBFS (next REC)",label,dim);
    for(int i=0;i<2;i++) {
        az_rect b={rx+5+i*((rw-10)/2),cy+button_y,(rw-14)/2,row-27};
        native_dynamics[1][i]=b;native_button(d,b,i ? "ON":"OFF",!!(flags&2)==i,1);
    }
#else
    panel_text(d,rx+5,cy+2,rw-10,"REC Limiter - under development",label,dim);
    native_button(d,(az_rect){rx+5,cy+button_y,(rw-14)/2,row-27},"OFF",0,0);
    native_button(d,(az_rect){rx+rw/2+2,cy+button_y,(rw-14)/2,row-27},"ON",0,0);
#endif
    native_hits=1;
}
static int unified_input(const az_event *ev,void *user) {
    (void)user;if(!ev)return 0;
    if(ev->kind==AZ_EV_BACK){sr_monitor_leave(&sound_monitor);return 0;}
    if(ev->kind==AZ_EV_KNOB) {selected=ev->dir>0 ? 1:0;C->request_redraw();return 1;}
    if(ev->kind==AZ_EV_CLICK){choose_bits(selected ? 16:24);return 1;}
    if(ev->kind==AZ_EV_TAP && native_hits)for(int i=0;i<2;i++) {
        az_rect b=native_bits[i];
        if(ev->x>=b.x && ev->x<b.x+b.w && ev->y>=b.y && ev->y<b.y+b.h) {
            selected=i;choose_bits(i ? 16:24);return 1;
        }
    }
#ifdef SR_NATIVE_HEADROOM
    if(ev->kind==AZ_EV_TAP && native_hits)for(int i=0;i<5;i++) {
        az_rect b=native_headroom[i];
        if(ev->x>=b.x && ev->x<b.x+b.w && ev->y>=b.y && ev->y<b.y+b.h) {
            choose_headroom(i*3);return 1;
        }
    }
#endif
#ifdef SR_NATIVE_DYNAMICS
    if(ev->kind==AZ_EV_TAP && native_hits)for(int group=0;group<2;group++)for(int i=0;i<2;i++) {
        az_rect b=native_dynamics[group][i];
        if(ev->x>=b.x && ev->x<b.x+b.w && ev->y>=b.y && ev->y<b.y+b.h) {
            choose_dynamics(group,i);return 1;
        }
    }
#endif
    return 0;
}
static void unified_enter(void *user) {
    (void)user;native_hits=0;
    selected=atomic_load_explicit(&sound_rec_requested_bits,memory_order_acquire)==16;
    sr_monitor_enter(&sound_monitor);
}
static const az_screen_v1 UNIFIED_PAGE={.size=sizeof(az_screen_v1),
    .on_draw=unified_draw,.on_input=unified_input,.on_enter=unified_enter};
static void unified_tick(void){
    sr_monitor_tick(&sound_monitor);
#ifdef SR_NATIVE_PROBE
    sr_native_probe_tick(C,MOD_ID);
#endif
}
