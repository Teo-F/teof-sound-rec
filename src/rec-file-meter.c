/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE
#define _GNU_SOURCE
#include "rec-file-meter.h"
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
int sr_meter_pcm(const unsigned char *pcm,size_t frames,unsigned bits,sr_file_meter *out) {
    if(!out)return 0;
    *out=(sr_file_meter){0};
    if(!pcm || !frames || frames>SR_METER_FRAMES || (bits!=16 && bits!=24))return 0;
    unsigned width=bits/8;
    double peak[2]={0},power[2]={0};
    int32_t scale=(int32_t)(1u<<(bits-1));
    for(size_t f=0;f<frames;f++)for(unsigned c=0;c<2;c++) {
        const unsigned char *p=pcm+(f*2+c)*width;
        uint32_t raw=p[0]|((uint32_t)p[1]<<8);
        if(width==3)raw|=(uint32_t)p[2]<<16;
        int32_t sample=raw>=(uint32_t)scale ? (int32_t)raw-2*scale : (int32_t)raw;
        double v=(double)sample/scale,a=fabs(v);
        if(a>peak[c])peak[c]=a;
        power[c]+=v*v;
        if(sample==-scale || sample==scale-1)out->fullscale[c]++;
    }
    for(unsigned c=0;c<2;c++) {
        out->peak_dbfs[c]=peak[c]>0 ? 20*log10(peak[c]) : -INFINITY;
        out->rms_dbfs[c]=power[c]>0 ? 10*log10(power[c]/frames) : -INFINITY;
    }
    out->frames=frames;return 1;
}
int sr_meter_file(const sr_observation *o,sr_file_meter *out) {
    if(!out)return 0;
    *out=(sr_file_meter){0};
    if(!o || !memchr(o->path,0,sizeof(o->path)) || o->path[0]!='/')return 0;
    sr_wav w;
    if(sr_wav_read(o->path,1,&w) || w.device!=o->wav.device || w.inode!=o->wav.inode ||
       w.channels!=2 || w.rate!=o->wav.rate || w.bits!=o->wav.bits ||
       (w.bits!=16 && w.bits!=24) || !w.frames)return 0;
    uint64_t frames=w.frames>SR_METER_FRAMES ? SR_METER_FRAMES : w.frames;
    unsigned align=2*(w.bits/8);
    uint64_t offset=w.data_offset+(w.frames-frames)*align;
    size_t count=(size_t)frames*align;
    unsigned char buffer[SR_METER_FRAMES*6];
    int fd=open(o->path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
    if(fd<0)return 0;
    struct stat st,after;int ok=0;
    if(fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_size<0 ||
       (uint64_t)st.st_dev!=w.device || (uint64_t)st.st_ino!=w.inode ||
       offset>(uint64_t)st.st_size || count>(uint64_t)st.st_size-offset)goto done;
    size_t n=0;
    while(n<count) {
        ssize_t r=pread(fd,buffer+n,count-n,(off_t)(offset+n));
        if(r<0 && errno==EINTR)continue;
        if(r<=0)goto done;
        n+=(size_t)r;
    }
    if(fstat(fd,&after) || after.st_dev!=st.st_dev || after.st_ino!=st.st_ino ||
       after.st_size<st.st_size)goto done;
    ok=sr_meter_pcm(buffer,(size_t)frames,w.bits,out);
    if(ok) {out->end_frame=w.frames;out->rate=w.rate;}
done:
    close(fd);return ok;
}
