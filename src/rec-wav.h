/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_WAV_H
#define REC_WAV_H
#include <stdint.h>
typedef struct {
    unsigned rate, bits, channels;
    uint64_t device,inode,file_bytes,data_bytes,frames,data_offset;
    double seconds;
    int finalized_structure;
} sr_wav;
/* Reads only RIFF chunk headers/fmt payload using independent read-only open.
 * active=1 allows stale RIFF/data lengths, reports only verified format and
 * available whole frames, never claims finalization. */
int sr_wav_read(const char *path,int active,sr_wav *out);
#endif
