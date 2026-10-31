/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    float l[]={0.f,-0.f,1.f,-1.f,1.5f,.01f},r[]={.1f,.2f,.3f,.4f,.5f,.6f};
    float a[6],b[6],copy[6]; memcpy(copy,l,sizeof l);
    for (int bits=16;bits<=24;bits+=8) for(int db=0;db<=12;db+=3) {
        sr_session s={0}; sr_format f={44100,2,(unsigned)bits,db};
        assert(!sr_start(&s,f)); assert(sr_start(&s,f)==-1);
        assert(!sr_process(&s,l,r,a,b,6));
        assert(!memcmp(copy,l,sizeof l)); assert(s.frames==6 && s.clips==3);
        if (!db) assert(!memcmp(a,l,sizeof l));
        else for(int i=0;i<6;i++) assert(fabs(a[i]-l[i]*pow(10.,-db/20.))<1e-7);
        assert(sr_process(&s,l,r,l,b,6)==-1);
        assert(sr_process(&s,l,r,a,a,6)==-1);
        assert(sr_process(&s,l,r,a,b,SR_MAX_BLOCK+1)==-1);
        float bad[]={NAN}; uint64_t frames=s.frames;
        a[0]=99; assert(sr_process(&s,bad,r,a,b,1)==-2);
        assert(a[0]==99 && s.frames==frames);
        assert(fabs(sr_elapsed(&s)-6./44100)<1e-12);
        sr_stop(&s); assert(sr_process(&s,l,r,a,b,6)==-1);
        assert(sr_space_seconds(f,44100*2*(bits/8)*10+4096,4096)==10);
        assert(!sr_space_seconds(f,1,4096));
    }
    sr_session s={0}; assert(sr_start(&s,(sr_format){96000,2,24,0})==-1);
    assert(sr_prefix_valid("TEOF-Live_01")); assert(sr_prefix_valid(""));
    assert(!sr_prefix_valid("../REC")); assert(!sr_prefix_valid("a/b"));
    assert(!sr_prefix_valid("1234567890123456789012345"));
    puts("rec-engine: gain, bypass, meters, session, space and prefix tests passed");
}
