/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-pipeline.h"
#include <assert.h>
#include <stdio.h>
typedef struct {unsigned calls;size_t frames;int fail;} sink_state;
static int sink(void *v,const float *l,const float *r,size_t n) {
    sink_state *s=v; assert(l && r);s->calls++;
    if(s->fail)return -1;
    s->frames+=n;return 0;
}
int main(void) {
    sr_pipeline p={0};sink_state a={0},b={0};
    sr_format f={44100,2,24,6};sr_processing controls={1,1};
    assert(!sr_pipeline_start(&p,f,controls,sink,&a));
    f.bits=16;controls.limiter=0; /* next preference cannot alter current */
    float l[]={2.f,.1f},r[]={1.f,.05f};
    assert(!sr_pipeline_write(&p,l,r,2));
    double envelope=p.session.limiter_gain;
    assert(!sr_pipeline_split(&p,sink,&b));
    assert(p.part==2 && p.session.limiter_gain==envelope);
    assert(p.session.format.bits==24 && p.session.processing.limiter==1);
    assert(!sr_pipeline_write(&p,l,r,2));
    assert(a.frames==2 && b.frames==2 && p.session.frames==4);
    b.fail=1;assert(sr_pipeline_write(&p,l,r,2)==-3);
    assert(p.failed && p.session.frames==4);
    unsigned calls=b.calls;assert(sr_pipeline_write(&p,l,r,2)==-1);
    assert(calls==b.calls && sr_pipeline_split(&p,sink,&a)==-1);
    sr_pipeline_stop(&p);
    assert(!sr_pipeline_start(&p,f,controls,sink,&a));
    assert(!p.failed && p.session.frames==0 && p.session.format.bits==16);
    assert(!p.session.processing.limiter);
    puts("pipeline: frozen settings, split continuity, failed-sink latch, next session passed");
}
