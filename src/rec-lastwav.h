/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_LASTWAV_H
#define REC_LASTWAV_H
#include "rec-observer.h"
/* Worker-only. Accept only the exact .WAV counterpart of a previously observed
 * .tmp after a scan found no open REC candidates. No newest-file search.
 * Returns1 for consistent structure and matching identity,0 unknown.
 * This does not prove successful native close, durability or audio fidelity. */
int sr_lastwav_check(const sr_observation *previous,int scan_result,
                     sr_observation *out);
#endif
