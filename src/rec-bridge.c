/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-bridge.h"
static int converted(void *ctx,const float *l,const float *r,size_t n) {
    sr_bridge *b=ctx;const float *planes[]={l,r,0};
    return b->convert(b->context,b->bound.writer,planes,2,(int)n)?0:-1;
}
int sr_bridge_bind(sr_bridge *b,const sr_identity *id,const sr_file_identity *t,
                   sr_processing controls,sr_convert convert,void *ctx) {
    if(!b || b->pipeline.session.active || !convert || !sr_identity_matches(id,t))return -1;
    /* Use authoritative stored format/path, not mutable caller token fields. */
    sr_pipeline pipeline={0};
    if(sr_pipeline_start(&pipeline,id->active.format,controls,converted,b))return -1;
    b->pipeline=pipeline;b->bound=id->active;b->convert=convert;b->context=ctx;
    return 0;
}
int sr_bridge_split(sr_bridge *b,const sr_identity *id,const sr_file_identity *t) {
    if(!b || !b->pipeline.session.active || b->pipeline.failed ||
       !sr_identity_matches(id,t) || b->bound.session!=id->active.session)return -1;
    if(b->bound.birth==id->active.birth)return -1;
    sr_format old=b->pipeline.session.format,next=id->active.format;
    if(old.rate!=next.rate || old.channels!=next.channels || old.bits!=next.bits ||
       old.headroom_db!=next.headroom_db)return -1;
    if(sr_pipeline_split(&b->pipeline,converted,b))return -1;
    b->bound=id->active;return 0;
}
bool sr_bridge_write(sr_bridge *b,const sr_identity *id,uintptr_t writer,
                     const float *const *planes,int channels,int n) {
    if(!b || !b->convert)return false;
    /* Unknown/stale/unbound writers go through the original callback untouched.
     * Do not inspect even the plane array on these paths. */
    if(writer!=b->bound.writer || !sr_identity_matches(id,&b->bound) ||
       !b->pipeline.session.active)
        return b->convert(b->context,writer,planes,channels,n);
    if(b->pipeline.failed)return false;
    if(channels!=2 || n<0 || (n && (!planes || !planes[0] || !planes[1]))) {
        b->pipeline.failed=1;return false;
    }
    for(int off=0;off<n;) {
        int count=n-off<SR_MAX_BLOCK?n-off:SR_MAX_BLOCK;
        if(sr_pipeline_write(&b->pipeline,planes[0]+off,planes[1]+off,(size_t)count)) {
            b->pipeline.failed=1;return false;
        }
        off+=count;
    }
    return true;
}
void sr_bridge_stop(sr_bridge *b) {
    if(b){sr_pipeline_stop(&b->pipeline);b->bound=(sr_file_identity){0};}
}
