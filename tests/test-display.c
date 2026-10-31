/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-display.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    sr_display d; sr_observation o={0};
    sr_display_build(0,0,0,&d); assert(strstr(d.format,"unavailable"));
    o.wav.rate=44100; o.wav.channels=2; o.wav.bits=24;
    o.wav.frames=4641744; o.wav.inode=o.storage.inode=23854;
    o.wav.device=o.storage.device=8;
    o.storage.available_bytes=UINT64_C(264600)*7200+1000;
    sr_display_build(1,&o,1000,&d);
    assert(strstr(d.format,"24-bit / 44100 Hz / 2 ch"));
    assert(!strcmp(d.elapsed,"Written: 0:01:45"));
    assert(strstr(d.remaining,"~2h 00m"));
    o.wav.bits=16; sr_display_build(1,&o,1000,&d);
    assert(strstr(d.remaining,"~3h 00m"));
    o.wav.finalized_structure=1; sr_display_build(1,&o,0,&d);
    assert(strstr(d.status,"unavailable"));
    puts("display: actual file format, written duration, reserve and unavailable state passed");
}
