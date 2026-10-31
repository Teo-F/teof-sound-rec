/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rec-storage.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <string.h>
int sr_storage_read(const char *path, sr_storage *out) {
    if (!out) return -1;
    memset(out,0,sizeof *out);
    if (!path || path[0]!='/' || strlen(path)>4095) return -1;
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0)return -1;
    struct stat st;struct statvfs fs;
    int rc=-1;
    if(!fstat(fd,&st) && S_ISREG(st.st_mode) && st.st_size>=0 &&
       !fstatvfs(fd,&fs)) {
        uint64_t unit=fs.f_frsize?fs.f_frsize:fs.f_bsize;
        uint64_t blocks=fs.f_bavail;
        if(unit && blocks<=UINT64_MAX/unit) {
            *out=(sr_storage){(uint64_t)st.st_dev,(uint64_t)st.st_ino,
                             (uint64_t)st.st_size,blocks*unit};
            rc=0;
        }
    }
    close(fd);
    return rc;
}
