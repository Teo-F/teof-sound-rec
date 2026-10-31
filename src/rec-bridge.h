/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_BRIDGE_H
#define REC_BRIDGE_H
#include <stdbool.h>
#include "rec-identity.h"
#include "rec-pipeline.h"
/* Application callback contract, NOT a native EP147 function signature. */
typedef bool (*sr_convert)(void *context,uintptr_t writer,
                          const float *const *planes,int channels,int frames);
typedef struct {
    sr_pipeline pipeline;
    sr_file_identity bound;
    sr_convert convert;
    void *context;
} sr_bridge;
int sr_bridge_bind(sr_bridge *b,const sr_identity *identity,
                   const sr_file_identity *token,sr_processing processing,
                   sr_convert convert,void *context);
int sr_bridge_split(sr_bridge *b,const sr_identity *identity,
                    const sr_file_identity *new_token);
bool sr_bridge_write(sr_bridge *b,const sr_identity *identity,uintptr_t writer,
                     const float *const *planes,int channels,int frames);
void sr_bridge_stop(sr_bridge *b);
#endif
