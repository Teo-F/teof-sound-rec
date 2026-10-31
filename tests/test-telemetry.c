/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-telemetry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static sr_telemetry state;
static void *producer(void *unused) {
    (void)unused;
    sr_observation o={0};
    strcpy(o.path,"/media/usb/test/ALPHATHETA REC/REC001.tmp");
    o.wav.rate=44100; o.wav.bits=24; o.wav.channels=2;
    o.wav.device=o.storage.device=8;
    for(unsigned i=1;i<10000;i++) {
        o.wav.inode=o.storage.inode=i;
        o.wav.frames=i*44100u;
        sr_telemetry_publish(&state,1,&o,100);
    }
    return 0;
}
int main(void) {
    sr_observation out,o={0}; pthread_t thread;
    assert(!sr_telemetry_init(&state));
    assert(!sr_telemetry_read(&state,100,&out));
    assert(!pthread_create(&thread,0,producer,0));
    for(unsigned i=0;i<10000;i++) if(sr_telemetry_read(&state,100,&out)) {
        assert(out.wav.inode==out.storage.inode);
        assert(out.wav.frames==out.wav.inode*44100u);
    }
    assert(!pthread_join(thread,0));
    assert(sr_telemetry_read(&state,1600,&out));
    assert(!sr_telemetry_read(&state,1601,&out)); assert(!out.path[0]);
    assert(!sr_telemetry_read(&state,99,&out));
    assert(sr_telemetry_read(&state,100,&o));
    sr_telemetry_publish(&state,2,&o,100);
    assert(!sr_telemetry_read(&state,100,&out));
    o.storage.inode++;
    sr_telemetry_publish(&state,1,&o,100);
    assert(!sr_telemetry_read(&state,100,&out));
    sr_telemetry_destroy(&state);
    puts("telemetry: coherent concurrent copies, stale/clock/ambiguity/identity refusal passed");
}
