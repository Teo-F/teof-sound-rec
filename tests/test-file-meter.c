/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-file-meter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
    sr_file_meter m;
    unsigned char silence[12]={0};
    assert(sr_meter_pcm(silence,2,24,&m));
    assert(isinf(m.peak_dbfs[0]) && m.peak_dbfs[0]<0 && isinf(m.rms_dbfs[1]));
    unsigned char half16[]={0,0x40,0,0xc0};
    assert(sr_meter_pcm(half16,1,16,&m));
    assert(fabs(m.peak_dbfs[0]+6.020599913)<1e-8);
    assert(m.peak_dbfs[0]==m.peak_dbfs[1] && m.rms_dbfs[0]==m.peak_dbfs[0]);
    unsigned char full24[]={0,0,0x80,0xff,0xff,0x7f};
    assert(sr_meter_pcm(full24,1,24,&m));
    assert(m.peak_dbfs[0]==0 && m.peak_dbfs[1]<0);
    assert(m.fullscale[0]==1 && m.fullscale[1]==1);
    assert(!sr_meter_pcm(full24,1,32,&m));assert(!m.frames);
    assert(!sr_meter_pcm(silence,SR_METER_FRAMES+1,16,&m));
    puts("file meter: signed16/24, stereo, silence, fullscale and bounds passed");
}
