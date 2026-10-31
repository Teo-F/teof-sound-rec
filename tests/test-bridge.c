#include "rec-bridge.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef struct {int calls,frames,fail;uintptr_t writer;} sink;
static bool convert(void *ctx,uintptr_t writer,const float *const *p,int ch,int n) {
    sink *s=ctx;s->calls++;s->writer=writer;
    if(writer==999) {assert(p==(void *)1 && ch==17 && n==-9);return true;}
    assert(ch==2 && p && p[0] && p[1]);
    if(s->fail)return false;
    for(int i=0;i<n;i++)assert(fabs(p[0][i])<.892);
    s->frames+=n;return true;
}
int main(void) {
    sr_identity id={0};sr_file_identity t,next;sr_bridge b={0};sink s={0};
    assert(!sr_identity_begin(&id));sr_format f={44100,2,24,0};
    assert(!sr_identity_stage(&id,100,f,"/usb/REC001.WAV",&t));
    assert(sr_bridge_bind(&b,&id,&t,(sr_processing){0,1},convert,&s)==-1);
    assert(!sr_identity_activate(&id,&t));
    assert(!sr_bridge_bind(&b,&id,&t,(sr_processing){0,1},convert,&s));
    float l[600],r[600];for(int i=0;i<600;i++){l[i]=2;r[i]=1;}
    const float *planes[]={l,r,0};
    assert(sr_bridge_write(&b,&id,999,(void *)1,17,-9)); /* no dereference */
    assert(sr_bridge_write(&b,&id,100,planes,2,600));
    assert(s.frames==600 && b.pipeline.session.frames==600 && s.calls==4);
    assert(l[0]==2 && r[0]==1);
    double gain=b.pipeline.session.limiter_gain;
    assert(!sr_identity_stage(&id,101,f,"/usb/REC002.WAV",&next));
    assert(!sr_identity_activate(&id,&next));
    assert(!sr_bridge_split(&b,&id,&next));
    assert(sr_bridge_split(&b,&id,&next)==-1);
    assert(b.pipeline.session.limiter_gain==gain);
    assert(sr_bridge_write(&b,&id,101,planes,2,2));assert(s.writer==101);
    s.fail=1;assert(!sr_bridge_write(&b,&id,101,planes,2,2));
    int calls=s.calls;assert(!sr_bridge_write(&b,&id,101,planes,2,2));
    assert(s.calls==calls && b.pipeline.session.frames==602);
    sr_bridge_stop(&b);sr_identity_end(&id);
    assert(sr_bridge_write(&b,&id,999,(void *)1,17,-9));
    puts("bridge: original bypass, identity gate, copied chunks, split continuity, failure latch passed");
}
