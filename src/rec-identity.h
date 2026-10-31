/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_IDENTITY_H
#define REC_IDENTITY_H
#include <stdint.h>
#include "rec-engine.h"
#define SR_PATH_MAX 1024
typedef struct {
    uint64_t session, birth;
    uintptr_t writer;
    sr_format format;
    char path[SR_PATH_MAX];
} sr_file_identity;
typedef struct {
    uint64_t generation, serial;
    int open;
    sr_file_identity active, pending;
} sr_identity;
/* Owner-thread event model only. Events require an independently proven native
 * lifetime lease. Integers never dereferenced; this does not create that lease. */
int sr_identity_begin(sr_identity *s);
int sr_identity_stage(sr_identity *s, uintptr_t writer, sr_format actual,
                      const char *copied_absolute_path, sr_file_identity *token);
int sr_identity_activate(sr_identity *s, const sr_file_identity *token);
int sr_identity_matches(const sr_identity *s, const sr_file_identity *token);
int sr_identity_retire(sr_identity *s, const sr_file_identity *token);
void sr_identity_end(sr_identity *s);
#endif
