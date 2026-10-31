/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_HEADROOM_COPY_HPP
#define REC_HEADROOM_COPY_HPP
#include "rec-native-probe.hpp"
#include <cmath>
#include <algorithm>
/* Call only after the exact writer/call-scope gate. Stereo helper's native
 * integer packer uses4096/channels frames; preserve2048-frame sink boundaries.
 * No dynamic allocation. Original returns false => stop, never retry. */
inline bool sr_headroom_convert(sr_native_convert_fn original,void *writer,
                               const float *const *planes,int ch,int frames,int db) {
    if(!original)return false;
    if(db==0 || ch!=2 || frames<=0 || (db!=3 && db!=6 && db!=9 && db!=12) ||
       !planes || !planes[0] || !planes[1])return original(writer,planes,ch,frames);
    /* Preserve original behavior for nonfinite sources rather than introduce
     * a new failure or partial processed write before discovering a NaN. */
    for(int c=0;c<2;c++)for(int f=0;f<frames;f++)
        if(!std::isfinite(planes[c][f]))return original(writer,planes,ch,frames);
    constexpr float gains[]={1.0f,0.70794578438f,0.50118723363f,0.35481338923f,0.25118864315f};
    const float gain=gains[db/3];
    alignas(16) float copied[2][2048];
    const float *output[]={copied[0],copied[1]};
    int offset=0;
    while(offset<frames) {
        int n=std::min(2048,frames-offset);
        for(int c=0;c<2;c++)for(int f=0;f<n;f++)copied[c][f]=planes[c][offset+f]*gain;
        if(!original(writer,output,ch,n))return false;
        offset+=n;
    }
    return true;
}
#endif
