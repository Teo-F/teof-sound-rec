#define _GNU_SOURCE
#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rec-observer.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void p32(unsigned char *p,unsigned v){for(int i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
int main(void){
    char path[]="/tmp/sr-observe-XXXXXX";int fd=mkstemp(path);assert(fd>=0);
    unsigned char h[44]={0};memcpy(h,"RIFF",4);p32(h+4,36);memcpy(h+8,"WAVEfmt ",8);
    p32(h+16,16);h[20]=1;h[22]=2;p32(h+24,44100);p32(h+28,264600);
    h[32]=6;h[34]=24;memcpy(h+36,"data",4);
    assert(write(fd,h,44)==44);assert(!ftruncate(fd,644));
    assert(lseek(fd,123,SEEK_SET)==123);
    sr_observation o;assert(sr_observer_probe(fd,path,&o)==1);
    assert(o.wav.bits==24 && o.wav.frames==100 && !strcmp(o.path,path));
    assert(lseek(fd,0,SEEK_CUR)==123); /* original writer offset never moves */
    int ro=open(path,O_RDONLY);assert(ro>=0);
    assert(!sr_observer_probe(ro,path,&o) && !o.wav.bits);close(ro);
    char other[]="/tmp/sr-other-XXXXXX";int replacement=mkstemp(other);assert(replacement>=0);
    assert(write(replacement,h,44)==44);
    assert(!sr_observer_probe(fd,other,&o)); /* different inode, plausible header */
    close(fd);assert(!sr_observer_probe(fd,path,&o));
    close(replacement);unlink(other);unlink(path);
    puts("observer: writable descriptor identity, header/storage binding and unchanged writer offset passed");
}
