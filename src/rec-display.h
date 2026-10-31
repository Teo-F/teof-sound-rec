/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_DISPLAY_H
#define REC_DISPLAY_H
#include "rec-observer.h"
typedef struct {
    char status[96],format[96],elapsed[96],space[96],remaining[128];
} sr_display;
/* Pure UI formatting of an already copied fresh snapshot. No filesystem IO.
 * reserve_bytes is an explicit caller policy, not measured disk capacity. */
void sr_display_build(int available,const sr_observation *o,
                      uint64_t reserve_bytes,sr_display *out);
#endif
