/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_OBSERVER_H
#define REC_OBSERVER_H
#include "rec-wav.h"
#include "rec-storage.h"
#define SR_OBSERVER_PATH 4096
typedef struct {
    char path[SR_OBSERVER_PATH];
    sr_wav wav;
    sr_storage storage;
} sr_observation;
/* Metadata only on writer_fd. Content read by independent normal-path open.
 * Returns1 verified momentary candidate,0 invalid/unknown. Never closes writer_fd. */
int sr_observer_probe(int writer_fd,const char *copied_path,sr_observation *out);
/* Linux only, current process only, bounded2048 entries. Default native recorder
 * folder on /media/usb. 1=unique candidate,0=none,2=ambiguous,-1=unavailable.
 * Worker-only, no settings or source writes, no /proc/fd content reads. */
int sr_observer_scan(sr_observation *out);
#endif
