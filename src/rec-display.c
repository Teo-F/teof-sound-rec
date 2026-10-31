/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-display.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
void sr_display_build(int available,const sr_observation *o,
                      uint64_t reserve,sr_display *out) {
    memset(out,0,sizeof(*out));
    snprintf(out->status,sizeof(out->status),"Recording file: unavailable");
    snprintf(out->format,sizeof(out->format),"File format: unavailable");
    snprintf(out->elapsed,sizeof(out->elapsed),"Written duration: unavailable");
    snprintf(out->space,sizeof(out->space),"REC USB free space: unavailable");
    snprintf(out->remaining,sizeof(out->remaining),"Space estimate: unavailable");
    if(!available || !o || !o->wav.rate || !o->wav.channels ||
       !o->wav.bits || o->wav.bits%8 || o->wav.finalized_structure ||
       o->wav.device!=o->storage.device || o->wav.inode!=o->storage.inode) return;
    snprintf(out->status,sizeof(out->status),"Open recording file observed");
    snprintf(out->format,sizeof(out->format),"File: PCM %u-bit / %u Hz / %u ch",
             o->wav.bits,o->wav.rate,o->wav.channels);
    uint64_t seconds=o->wav.frames/o->wav.rate;
    snprintf(out->elapsed,sizeof(out->elapsed),"Written: %"PRIu64":%02"PRIu64":%02"PRIu64,
             seconds/3600,seconds/60%60,seconds%60);
    snprintf(out->space,sizeof(out->space),"REC USB free: %.2f GiB",
             (double)o->storage.available_bytes/1073741824.0);
    uint64_t bytes_per_second=(uint64_t)o->wav.rate*o->wav.channels*(o->wav.bits/8);
    seconds=o->storage.available_bytes>reserve ?
        (o->storage.available_bytes-reserve)/bytes_per_second : 0;
    snprintf(out->remaining,sizeof(out->remaining),
             "Space only: ~%"PRIu64"h %02"PRIu64"m (excludes split limits)",
             seconds/3600,seconds/60%60);
}
