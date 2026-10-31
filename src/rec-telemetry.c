/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-telemetry.h"
#include <string.h>
int sr_telemetry_init(sr_telemetry *t) {
    if (!t) return -1;
    memset(t,0,sizeof(*t));
    return pthread_mutex_init(&t->mutex,0);
}
void sr_telemetry_destroy(sr_telemetry *t) { pthread_mutex_destroy(&t->mutex); }
void sr_telemetry_publish(sr_telemetry *t,int result,
                          const sr_observation *o,uint64_t now) {
    pthread_mutex_lock(&t->mutex);
    memset(&t->observation,0,sizeof(t->observation));
    t->available=result==1 && o && o->path[0] &&
        memchr(o->path,0,sizeof(o->path)) && o->wav.rate &&
        o->wav.channels && o->wav.bits &&
        o->wav.device==o->storage.device && o->wav.inode==o->storage.inode;
    if(t->available) t->observation=*o;
    t->stamp_ms=now;
    pthread_mutex_unlock(&t->mutex);
}
int sr_telemetry_read(sr_telemetry *t,uint64_t now,sr_observation *out) {
    int valid;
    if(!out) return 0;
    memset(out,0,sizeof(*out));
    if(pthread_mutex_trylock(&t->mutex)) return 0;
    valid=t->available && now>=t->stamp_ms && now-t->stamp_ms<=1500;
    if(valid) *out=t->observation;
    pthread_mutex_unlock(&t->mutex);
    return valid;
}
