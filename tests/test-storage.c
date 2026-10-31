/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rec-storage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
int main(void) {
    char root[]="/tmp/sr-storage-XXXXXX";assert(mkdtemp(root));
    char file[128],link[128];snprintf(file,sizeof file,"%s/REC001.WAV",root);
    snprintf(link,sizeof link,"%s/link.wav",root);
    FILE *f=fopen(file,"wb");assert(f);assert(fwrite("test",1,4,f)==4);fclose(f);
    struct stat st;assert(!stat(file,&st));sr_storage out;
    assert(!sr_storage_read(file,&out));assert(out.file_bytes==4);
    assert(out.inode==(uint64_t)st.st_ino && out.device==(uint64_t)st.st_dev);
    assert(!symlink(file,link));assert(sr_storage_read(link,&out)==-1);
    assert(out.inode==0 && out.available_bytes==0);
    assert(sr_storage_read(root,&out)==-1);
    assert(sr_storage_read("relative.wav",&out)==-1);
    assert(sr_storage_read("/missing-sr-file.wav",&out)==-1);
    assert(!unlink(link));assert(!unlink(file));assert(!rmdir(root));
    puts("storage: file identity/size, same-fd filesystem space, symlink/directory/missing refusal passed");
}
