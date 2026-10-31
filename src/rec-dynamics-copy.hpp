/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_DYNAMICS_COPY_HPP
#define REC_DYNAMICS_COPY_HPP
#include "rec-native-probe.hpp"
#include "rec-headroom-copy.hpp"
#include "rec-dynamics-sessions.hpp"
#include <cmath>
#include <cfloat>
/* Offline adapter candidate; only a proven REC call-scope may invoke it.
 * Caller supplies an exclusively leased session, never UI-owned state.
 * Preserve native stereo2048-frame write boundaries, 256-frame DSP subblocks.
 * On sink failure latch this session inactive; never retry/advance another
 * processed chunk. ORIGINAL behavior is used for nonfinite whole calls. */
inline bool sr_dynamics_convert(sr_native_convert_fn original,void *writer,
    const float *const *planes,int ch,int frames,int db,sr_session *session) {
    if(!session || (!session->processing.auto_gain && !session->processing.limiter))
        return sr_headroom_convert(original,writer,planes,ch,frames,db);
    if(!session->active)return false;
    if(!original)return false;
    if(ch!=2 || frames<=0 || !planes || !planes[0] || !planes[1] ||
       db<0 || db>12 || db%3)return original(writer,planes,ch,frames);
    for(int c=0;c<2;c++)for(int f=0;f<frames;f++)
        if(!std::isfinite(planes[c][f]))return original(writer,planes,ch,frames);
    /* Without limiter the engine refuses float overflow. Preflight whole
     * call so an extreme later source cannot cause partially processed input. */
    if(session->processing.auto_gain && !session->processing.limiter)
        for(int c=0;c<2;c++)for(int f=0;f<frames;f++)
            if(std::fabs(double(planes[c][f]))>double(FLT_MAX)/2.0)
                return original(writer,planes,ch,frames);
    session->format.headroom_db=db;
    alignas(16) float copied[2][2048];const float *out[]={copied[0],copied[1]};
    for(int at=0;at<frames;) {
        int n=std::min(2048,frames-at);
        for(int sub=0;sub<n;) {
            int count=std::min(SR_MAX_BLOCK,n-sub);
            if(sr_process(session,planes[0]+at+sub,planes[1]+at+sub,
                          copied[0]+sub,copied[1]+sub,count)) {
                sr_stop(session);return false;
            }
            sub+=count;
        }
        if(!original(writer,out,ch,n)){sr_stop(session);return false;}
        at+=n;
    }
    return true;
}
#endif
