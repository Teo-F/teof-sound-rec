/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rec-observer.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
int sr_observer_probe(int writer_fd,const char *path,sr_observation *out){
    if(!out)return 0;
    *out=(sr_observation){0};
    if(writer_fd<0 || !path || path[0]!='/')return 0;
    size_t n=strlen(path);if(n>=SR_OBSERVER_PATH)return 0;
    int flags=fcntl(writer_fd,F_GETFL);
    if(flags<0 || (flags&O_ACCMODE)==O_RDONLY)return 0;
    struct stat before,after;
    if(fstat(writer_fd,&before) || !S_ISREG(before.st_mode))return 0;
    sr_observation o={0};
    if(sr_wav_read(path,1,&o.wav) || sr_storage_read(path,&o.storage))return 0;
    if(fstat(writer_fd,&after) || before.st_dev!=after.st_dev ||
       before.st_ino!=after.st_ino || (uint64_t)before.st_dev!=o.wav.device ||
       (uint64_t)before.st_ino!=o.wav.inode || o.wav.device!=o.storage.device ||
       o.wav.inode!=o.storage.inode)return 0;
    flags=fcntl(writer_fd,F_GETFL);
    if(flags<0 || (flags&O_ACCMODE)==O_RDONLY)return 0;
    memcpy(o.path,path,n+1);*out=o;return 1;
}
int sr_observer_scan(sr_observation *out){
    if(!out)return -1;
    *out=(sr_observation){0};
#ifdef __linux__
    DIR *d=opendir("/proc/self/fd");if(!d)return -1;
    int result=0;unsigned entries=0;struct dirent *e;sr_observation candidate={0};
    for(;;){
        errno=0;e=readdir(d);
        if(!e){if(errno)result=-1;break;}
        if(++entries>2048){result=-1;break;}
        char *end=0;long number=strtol(e->d_name,&end,10);
        if(!e->d_name[0] || *end || number<0 || number>INT_MAX)continue;
        int fd=(int)number,flags=fcntl(fd,F_GETFL);
        if(flags<0 || (flags&O_ACCMODE)==O_RDONLY)continue;
        char path[SR_OBSERVER_PATH],again[SR_OBSERVER_PATH];
        ssize_t n=readlinkat(dirfd(d),e->d_name,path,sizeof path-1);
        if(n<0 || n>=(ssize_t)sizeof path-1)continue;
        path[n]=0;
        if(strncmp(path,"/media/usb/",11) || !strstr(path,"/ALPHATHETA REC/"))continue;
        size_t len=(size_t)n;
        if(len<4 || (strcmp(path+len-4,".tmp") && strcmp(path+len-4,".WAV")))continue;
        sr_observation current;
        if(!sr_observer_probe(fd,path,&current)){result=-1;break;}
        n=readlinkat(dirfd(d),e->d_name,again,sizeof again-1);
        if(n<0 || n>=(ssize_t)sizeof again-1){result=-1;break;}
        again[n]=0;if(strcmp(path,again)){result=-1;break;}
        if(result==1){
            /* A second descriptor of same file does not create a second REC. */
            if(candidate.wav.device==current.wav.device && candidate.wav.inode==current.wav.inode)continue;
            result=2;break;
        }
        candidate=current;result=1;
    }
    closedir(d);
    if(result==1)*out=candidate;
    return result;
#else
    return -1;
#endif
}
