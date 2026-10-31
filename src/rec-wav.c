/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rec-wav.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
static unsigned u16(const unsigned char *p){return p[0]|((unsigned)p[1]<<8);}
static uint32_t u32(const unsigned char *p){return u16(p)|((uint32_t)u16(p+2)<<16);}
static int bytes(int fd,uint64_t off,unsigned char *p,size_t n){
    size_t done=0;
    while(done<n){ssize_t r=pread(fd,p+done,n-done,(off_t)(off+done));
        if(r<0 && errno==EINTR)continue;
        if(r<=0)return -1;
        done+=(size_t)r;
    }return 0;
}
int sr_wav_read(const char *path,int active,sr_wav *out){
    if(!out)return -1;
    *out=(sr_wav){0};
    if(!path || path[0]!='/' || (active!=0 && active!=1))return -1;
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0)return -1;
    int rc=-1;struct stat st,after;sr_wav w={0};unsigned char h[16];
    if(fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_size<12)goto end;
    uint64_t size=(uint64_t)st.st_size;
    if(bytes(fd,0,h,12) || memcmp(h,"RIFF",4) || memcmp(h+8,"WAVE",4))goto end;
    if(!active && (uint64_t)u32(h+4)+8!=size)goto end;
    uint64_t pos=12;unsigned align=0;int fmt=0,data=0;
    for(unsigned count=0;pos<size && count<4096;count++){
        if(size-pos<8 || bytes(fd,pos,h,8))goto end;
        uint64_t length=u32(h+4), payload=pos+8, next=payload+length+(length&1);
        if(!memcmp(h,"data",4)){
            if(data || !fmt)goto end;
            data=1;
            if(!active && next>size)goto end;
            uint64_t available=active?size-payload:length;
            if(!active && available%align)goto end;
            w.data_bytes=available;w.frames=available/align;w.data_offset=payload;
            if(active){pos=size;break;} /* never scan audio using stale length */
        }else if(!memcmp(h,"fmt ",4)){
            if(fmt || length<16 || next>size || bytes(fd,payload,h,16))goto end;
            w.channels=u16(h+2);w.rate=u32(h+4);align=u16(h+12);w.bits=u16(h+14);
            if(u16(h)!=1 || w.channels<1 || w.channels>32 || w.rate<8000 ||
               w.rate>384000 || (w.bits!=8 && w.bits!=16 && w.bits!=24 && w.bits!=32) ||
               align!=w.channels*(w.bits/8) || u32(h+8)!=w.rate*align)goto end;
            fmt=1;
        }else if(next>size)goto end;
        pos=next;
    }
    if(!fmt || !data || pos!=size || fstat(fd,&after))goto end;
    if(!active && (st.st_size!=after.st_size || st.st_mtime!=after.st_mtime))goto end;
    w.device=(uint64_t)st.st_dev;w.inode=(uint64_t)st.st_ino;
    w.file_bytes=size;w.seconds=(double)w.frames/w.rate;
    w.finalized_structure=!active;*out=w;rc=0;
end:
    close(fd);return rc;
}
