/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-identity.h"
#include <string.h>
int sr_identity_begin(sr_identity *s) {
    if(!s || s->open || s->generation==UINT64_MAX)return -1;
    s->generation++;s->open=1;
    s->active=(sr_file_identity){0};s->pending=(sr_file_identity){0};
    return 0;
}
int sr_identity_stage(sr_identity *s, uintptr_t writer, sr_format f,
                      const char *path,sr_file_identity *token) {
    if(token)*token=(sr_file_identity){0};
    if(!s || !s->open || !writer || !path || !token || path[0]!='/' ||
       !sr_format_valid(f) || s->serial==UINT64_MAX)return -1;
    if(writer==s->active.writer || writer==s->pending.writer)return -1;
    size_t n=0;
    while(n<SR_PATH_MAX && path[n])n++;
    if(!n || n==SR_PATH_MAX)return -1;
    sr_file_identity next={.session=s->generation,.birth=++s->serial,
                           .writer=writer,.format=f};
    memcpy(next.path,path,n+1);
    s->pending=next;*token=next;
    return 0;
}
static int same(const sr_file_identity *a,const sr_file_identity *b) {
    return a && b && a->writer && a->writer==b->writer &&
           a->session==b->session && a->birth==b->birth;
}
int sr_identity_activate(sr_identity *s,const sr_file_identity *token) {
    if(!s || !s->open || !same(&s->pending,token))return -1;
    s->active=s->pending;s->pending=(sr_file_identity){0};return 0;
}
int sr_identity_matches(const sr_identity *s,const sr_file_identity *token) {
    return s && s->open && same(&s->active,token);
}
int sr_identity_retire(sr_identity *s,const sr_file_identity *token) {
    if(!s || !s->open)return -1;
    if(same(&s->pending,token)) {s->pending=(sr_file_identity){0};return 0;}
    if(same(&s->active,token)) {s->active=(sr_file_identity){0};return 0;}
    return -1;
}
void sr_identity_end(sr_identity *s) {
    if(s){s->open=0;s->active=(sr_file_identity){0};s->pending=(sr_file_identity){0};}
}
