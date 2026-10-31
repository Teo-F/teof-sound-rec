/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-engine.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static sr_format fmt={44100,2,24,0};
int main(void) {
    float l[256],r[256],a[256],b[256],one[256],two[256];
    for(int i=0;i<256;i++) {l[i]=(i%7==0)?2.f:.5f; r[i]=l[i]*.25f;}
    for(int agc=0;agc<=1;agc++) {
        sr_session s={0}; assert(!sr_start_processing(&s,fmt,(sr_processing){agc,1}));
        assert(!sr_process(&s,l,r,a,b,256));
        for(int i=0;i<256;i++) {
            assert(fabs(a[i])<=0.8912509381337456);
            assert(fabs(b[i]-a[i]*.25)<1e-7); /* linked stereo image */
        }
        assert(s.reduction_db>0);
        sr_session p={0}; assert(!sr_start_processing(&p,fmt,(sr_processing){agc,1}));
        for(int i=0;i<256;i++) assert(!sr_process(&p,l+i,r+i,one+i,two+i,1));
        assert(!memcmp(a,one,sizeof a) && !memcmp(b,two,sizeof b));
    }
    sr_session s={0}; assert(!sr_start_processing(&s,fmt,(sr_processing){1,0}));
    memset(l,0,sizeof l);memset(r,0,sizeof r);
    for(int k=0;k<2000;k++) assert(!sr_process(&s,l,r,a,b,256));
    assert(s.agc_db==0 && s.rms_power==0);
    for(int i=0;i<256;i++) l[i]=r[i]=.01f;
    for(int k=0;k<3000;k++) assert(!sr_process(&s,l,r,a,b,256));
    assert(s.agc_db>5.9 && s.agc_db<=6.);
    double saved=s.agc_db;
    memset(l,0,sizeof l);memset(r,0,sizeof r);
    for(int k=0;k<100;k++) assert(!sr_process(&s,l,r,a,b,256));
    assert(s.agc_db==saved);
    for(int i=0;i<256;i++) l[i]=r[i]=1.f;
    for(int k=0;k<3000;k++) assert(!sr_process(&s,l,r,a,b,256));
    assert(s.agc_db < -17.9 && s.agc_db>=-18.);
    sr_session extreme={0};assert(!sr_start_processing(&extreme,fmt,(sr_processing){1,1}));
    l[0]=FLT_MAX;r[0]=-FLT_MAX;
    assert(!sr_process(&extreme,l,r,a,b,1));
    assert(isfinite(a[0]) && isfinite(b[0]) && fabs(a[0])<.892);
    sr_session copy=extreme;
    l[0]=INFINITY;
    assert(sr_process(&extreme,l,r,a,b,1)==-2);
    assert(!memcmp(&copy,&extreme,sizeof copy));
    puts("processing: ceiling, stereo link, block invariance, silence gate, gain bounds, overload passed");
}
