#define _GNU_SOURCE
#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rec-wav.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
static void p32(unsigned char *p,uint32_t v){for(int i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void fixture(unsigned char *h){
    memset(h,0,236);memcpy(h,"RIFF",4);p32(h+4,236+600-8);memcpy(h+8,"WAVEJUNK",8);
    p32(h+16,52);memcpy(h+72,"fmt ",4);p32(h+76,16);h[80]=1;h[82]=2;
    p32(h+84,44100);p32(h+88,264600);h[92]=6;h[94]=24;
    memcpy(h+96,"smpl",4);p32(h+100,60);memcpy(h+164,"LIST",4);p32(h+168,56);
    memcpy(h+228,"data",4);p32(h+232,600);
}
int main(void){
    char path[]="/tmp/sr-wav-XXXXXX";int fd=mkstemp(path);assert(fd>=0);
    unsigned char h[236];fixture(h);assert(write(fd,h,sizeof h)==sizeof h);
    assert(!ftruncate(fd,836));sr_wav w;
    assert(!sr_wav_read(path,0,&w));assert(w.bits==24 && w.rate==44100 && w.channels==2);
    assert(w.frames==100 && w.finalized_structure);
    assert(!sr_wav_read(path,1,&w) && !w.finalized_structure);
    /* In-progress native header sizes can lag real available sample data. */
    p32(h+4,100);p32(h+232,0);assert(pwrite(fd,h,sizeof h,0)==sizeof h);
    assert(sr_wav_read(path,0,&w)==-1 && w.frames==0);
    assert(!sr_wav_read(path,1,&w) && w.frames==100);
    /* Active half-frame allowed but not counted; final file rejects it. */
    assert(!ftruncate(fd,837));assert(!sr_wav_read(path,1,&w) && w.frames==100);
    fixture(h);p32(h+4,829);p32(h+232,601);assert(pwrite(fd,h,sizeof h,0)==sizeof h);
    assert(sr_wav_read(path,0,&w)==-1);
    fixture(h);h[80]=3;assert(pwrite(fd,h,sizeof h,0)==sizeof h);
    assert(sr_wav_read(path,1,&w)==-1 && !w.rate);
    fixture(h);p32(h+16,0xffffffff);assert(pwrite(fd,h,sizeof h,0)==sizeof h);
    assert(sr_wav_read(path,1,&w)==-1);
    assert(sr_wav_read("relative",1,&w)==-1);
    close(fd);unlink(path);
    puts("C WAV: native236byte layout, active stale sizes, final alignment, encoding and truncation passed");
}
