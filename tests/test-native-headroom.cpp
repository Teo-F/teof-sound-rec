/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-native-probe.hpp"
#include "rec-native-probe-api.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <limits>
#include <cstdio>
struct Worker {unsigned char padding[24];void *writer;};
static std::vector<float> left(4101),right(4101),written_l,written_r;
static const float *planes[]={left.data(),right.data()};
static int calls,fail_on;
static bool exact,change_setting;
static bool sink(void *writer,const float *const *p,int ch,int frames) {
    assert(writer==reinterpret_cast<void *>(std::uintptr_t(42)) && ch==2);
    calls++;
    if(exact)assert(p==planes && frames==4101);
    else assert(p!=planes && frames<=2048);
    if(change_setting && calls==1)sr_native_headroom_set(12);
    if(calls==fail_on)return false;
    written_l.insert(written_l.end(),p[0],p[0]+frames);
    written_r.insert(written_r.end(),p[1],p[1]+frames);
    return true;
}
static int worker(void *w) {
    return sr_native_convert_probe(static_cast<Worker *>(w)->writer,planes,2,4101) ? 7:10;
}
static void reset(bool original_pointer) {
    calls=0;fail_on=0;exact=original_pointer;change_setting=false;
    written_l.clear();written_r.clear();
}
int main() {
    for(unsigned i=0;i<left.size();i++){left[i]=0.5f*std::sin(float(i));right[i]=-left[i];}
    auto preserved_l=left,preserved_r=right;
    Worker w={{0},reinterpret_cast<void *>(std::uintptr_t(42))};
    sr_native_probe_set_originals(worker,sink);
    assert(sr_native_headroom_get()==0 && !sr_native_headroom_set(2));
    sr_native_headroom_set(6);reset(true);
    assert(sr_native_worker_probe(&w)==7 && calls==1); /* disabled until full init */
    sr_native_headroom_enable(1);
    for(int db: {0,3,6,9,12}) {
        sr_native_headroom_set(db);reset(db==0);
        assert(sr_native_worker_probe(&w)==7);
        assert(calls==(db ? 3:1) && written_l.size()==left.size());
        double gain=std::pow(10.0,-double(db)/20);
        for(unsigned i=0;i<left.size();i++) {
            assert(std::abs(written_l[i]-left[i]*gain)<1e-7);
            assert(std::abs(written_r[i]-right[i]*gain)<1e-7);
        }
        assert(!std::memcmp(left.data(),preserved_l.data(),left.size()*sizeof(float)));
        assert(!std::memcmp(right.data(),preserved_r.data(),right.size()*sizeof(float)));
    }
    sr_native_headroom_set(6);reset(true);
    assert(sr_native_convert_probe(w.writer,planes,2,4101) && calls==1); /* outside REC unchanged */
    reset(false);fail_on=2;
    assert(sr_native_worker_probe(&w)==10 && calls==2 && written_l.size()==2048);
    reset(false);change_setting=true;
    assert(sr_native_worker_probe(&w)==7);
    for(unsigned i=0;i<left.size();i++)assert(std::abs(written_l[i]-left[i]*std::pow(10.0,-6.0/20))<1e-7);
    sr_native_headroom_set(6);left.back()=std::numeric_limits<float>::quiet_NaN();reset(true);
    assert(sr_native_worker_probe(&w)==7 && calls==1); /* whole-call fallback before any write */
    puts("native headroom: default bypass, five gains, private planes, native2048 chunks, scope, failure and call snapshot passed");
}
