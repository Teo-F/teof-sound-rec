/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-lastwav.h"
#include <string.h>
int sr_lastwav_check(const sr_observation *p,int result,sr_observation *out) {
    if(!out) return 0;
    *out=(sr_observation){0};
    if(!p || result!=0 || !memchr(p->path,0,sizeof(p->path))) return 0;
    size_t n=strlen(p->path);
    if(n<5 || p->path[0]!='/' || strcmp(p->path+n-4,".tmp") ||
       p->wav.finalized_structure || !p->wav.rate ||
       p->wav.device!=p->storage.device || p->wav.inode!=p->storage.inode) return 0;
    sr_observation final={0};
    memcpy(final.path,p->path,n+1);memcpy(final.path+n-4,".WAV",5);
    if(sr_wav_read(final.path,0,&final.wav) ||
       sr_storage_read(final.path,&final.storage)) return 0;
    if(final.wav.device!=p->wav.device || final.wav.inode!=p->wav.inode ||
       final.storage.device!=final.wav.device || final.storage.inode!=final.wav.inode ||
       final.storage.file_bytes!=final.wav.file_bytes ||
       final.wav.rate!=p->wav.rate || final.wav.bits!=p->wav.bits ||
       final.wav.channels!=p->wav.channels || final.wav.frames<p->wav.frames) return 0;
    *out=final;return 1;
}
