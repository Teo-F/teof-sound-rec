/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-pipeline.h"
#include <limits.h>
int sr_pipeline_start(sr_pipeline *p, sr_format f, sr_processing settings,
                      sr_sink sink, void *context) {
    if (!p || p->session.active || !sink) return -1;
    sr_session s={0};
    if (sr_start_processing(&s,f,settings)) return -1;
    *p=(sr_pipeline){.session=s,.sink=sink,.context=context,.part=1};
    return 0;
}
int sr_pipeline_write(sr_pipeline *p, const float *l, const float *r, size_t n) {
    if (!p || p->failed || !p->session.active || !p->sink) return -1;
    sr_session next=p->session;
    int rc=sr_process(&next,l,r,p->left,p->right,n);
    if (rc) return rc;
    if (n && p->sink(p->context,p->left,p->right,n)) {
        /* Ambiguous/partial sink delivery cannot be rolled back. Latch failure,
         * do not retry, do not report these frames as successfully delivered. */
        p->failed=1; return -3;
    }
    p->session=next;
    return 0;
}
int sr_pipeline_split(sr_pipeline *p, sr_sink sink, void *context) {
    if (!p || p->failed || !p->session.active || !sink || p->part==UINT_MAX) return -1;
    /* Caller must first confirm old sink finalized and new sink opened.
     * Settings, frame time, AGC and release envelope survive track marks. */
    p->sink=sink;p->context=context;p->part++;
    return 0;
}
void sr_pipeline_stop(sr_pipeline *p) {
    if(p) {sr_stop(&p->session);p->sink=0;p->context=0;}
}
