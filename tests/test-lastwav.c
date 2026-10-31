/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE
#define _GNU_SOURCE
#include "rec-lastwav.h"
#include "rec-file-meter.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
static void put32(unsigned char *p,unsigned v) {
    for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));
}
int main(void) {
    char dir[]="/tmp/rec-lastwav-XXXXXX"; assert(mkdtemp(dir));
    char tmp[256],wav[256];snprintf(tmp,sizeof(tmp),"%s/REC001.tmp",dir);
    snprintf(wav,sizeof(wav),"%s/REC001.WAV",dir);
    unsigned char h[48]={0};
    memcpy(h,"RIFF",4);put32(h+4,40);memcpy(h+8,"WAVEfmt ",8);
    put32(h+16,16);h[20]=1;h[22]=2;put32(h+24,44100);
    put32(h+28,176400);h[32]=4;h[34]=16;
    memcpy(h+36,"data",4);put32(h+40,4);
    int fd=open(tmp,O_CREAT|O_EXCL|O_RDWR,0600);assert(fd>=0);
    assert(write(fd,h,sizeof h)==sizeof h);
    sr_observation previous={0},final;
    strcpy(previous.path,tmp);
    assert(!sr_wav_read(tmp,1,&previous.wav));
    assert(!sr_storage_read(tmp,&previous.storage));
    sr_file_meter meter;
    off_t offset=lseek(fd,0,SEEK_CUR);
    assert(sr_meter_file(&previous,&meter) && meter.frames==1);
    assert(lseek(fd,0,SEEK_CUR)==offset); /* independent read leaves writer alone */
    assert(!sr_lastwav_check(&previous,0,&final)); /* not renamed */
    assert(!close(fd));assert(!rename(tmp,wav));
    assert(!sr_meter_file(&previous,&meter)); /* original pathname no longer exists */
    assert(!sr_lastwav_check(&previous,1,&final));
    assert(!sr_lastwav_check(&previous,2,&final));
    assert(!sr_lastwav_check(&previous,-1,&final));
    assert(sr_lastwav_check(&previous,0,&final));
    assert(final.wav.finalized_structure && final.wav.frames==1);
    previous.wav.inode++;assert(!sr_lastwav_check(&previous,0,&final));
    previous.wav.inode--;previous.wav.bits=24;
    assert(!sr_lastwav_check(&previous,0,&final));previous.wav.bits=16;
    previous.wav.frames=2;assert(!sr_lastwav_check(&previous,0,&final));
    previous.wav.frames=1;
    fd=open(wav,O_WRONLY);assert(fd>=0);h[4]=0;
    assert(write(fd,h,sizeof h)==sizeof h);close(fd);
    assert(!sr_lastwav_check(&previous,0,&final)); /* corrupt RIFF */
    assert(!final.path[0]);unlink(wav);rmdir(dir);
    puts("last WAV: exact rename/inode/format, scan ambiguity, truncated history and corruption passed");
}
