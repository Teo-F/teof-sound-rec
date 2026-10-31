/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_STORAGE_H
#define REC_STORAGE_H
#include <stdint.h>
typedef struct {
    uint64_t device, inode, file_bytes, available_bytes;
} sr_storage;
/* Worker-only read-only file open. Caller supplies a COPIED, verified native
 * recording path. No scan/newest-file guessing, retained descriptor or writes. */
int sr_storage_read(const char *absolute_wav, sr_storage *out);
#endif
