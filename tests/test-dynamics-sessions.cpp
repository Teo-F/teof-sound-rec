/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-dynamics-copy.hpp"
#include <cassert>
#include <vector>
#include <thread>
#include <cstdio>
#include <limits>
static std::vector<float> written_l,written_r;
static int sink_calls,fail_on;
static bool exact;
static const float *const *input;
static bool sink(void*,const float *const *p,int ch,int n) {
    assert(ch==2);++sink_calls;
    if(exact)assert(p==input);else assert(p!=input && n<=2048);
    if(sink_calls==fail_on)return false;
    written_l.insert(written_l.end(),p[0],p[0]+n);
    written_r.insert(written_r.end(),p[1],p[1]+n);return true;
}
static void clear(){written_l.clear();written_r.clear();sink_calls=fail_on=0;exact=false;}
int main() {
    sr::DynamicsSessions bank;
    auto t=bank.begin(42,{1,1});assert(t.generation);
    {
        auto a=bank.acquire(42);assert(a && a.engine()->agc_db==0 && a.engine()->limiter_gain==1);
        a.engine()->agc_db=3;
        assert(!bank.acquire(42));
        bool refused=false;std::thread thread([&]{refused=!bank.acquire(42);});thread.join();assert(refused);
        // Retire while in-flight: storage remains valid until lease release.
        bank.retire(t);assert(a.engine()->agc_db==3 && !bank.acquire(42));
        auto replacement=bank.begin(42,{0,1});assert(replacement.generation!=t.generation);
        auto b=bank.acquire(42);assert(b && b.engine()->agc_db==0);
        bank.retire(t); // Old generation cannot retire the replacement.
    }
    auto current=bank.current(42);assert(current.generation && current.generation!=t.generation);
    {
        auto lease=bank.acquire(42);assert(lease);
        lease.engine()->limiter_gain=.2;
    }
    {auto lease=bank.acquire(42);assert(lease.engine()->limiter_gain==.2);} // Sameworker/split continuity.
    bank.retire(current);assert(!bank.acquire(42));
    for(unsigned i=0;i<8;i++)assert(bank.begin(100+i,{0,0}).generation);
    assert(!bank.begin(999,{0,0}).generation); // Bounded, no eviction of live envelopes.
    assert(!bank.acquire(999));

    std::vector<float> l(4101,2.f),r(4101,.5f);const float *p[]={l.data(),r.data()};input=p;
    sr_session session{};assert(!sr_start_processing(&session,{44100,2,24,0},{0,1}));
    clear();assert(sr_dynamics_convert(sink,nullptr,p,2,4101,0,&session));
    assert(sink_calls==3 && written_l.size()==4101);
    for(unsigned i=0;i<l.size();i++) {
        assert(std::abs(written_l[i])<=.891251f && std::abs(written_r[i]-written_l[i]*.25f)<1e-7);
        assert(l[i]==2 && r[i]==.5f);
    }
    assert(session.reduction_db>6);
    sr_session whole{};assert(!sr_start_processing(&whole,{44100,2,24,0},{1,1}));
    clear();assert(sr_dynamics_convert(sink,nullptr,p,2,4101,6,&whole));auto wl=written_l,wr=written_r;
    sr_session split{};assert(!sr_start_processing(&split,{44100,2,24,0},{1,1}));clear();
    const float *first[]={l.data(),r.data()},*second[]={l.data()+137,r.data()+137};
    assert(sr_dynamics_convert(sink,nullptr,first,2,137,6,&split));
    assert(sr_dynamics_convert(sink,nullptr,second,2,3964,6,&split));
    assert(wl==written_l && wr==written_r && whole.agc_db==split.agc_db);
    sr_session failed{};assert(!sr_start_processing(&failed,{44100,2,24,0},{0,1}));
    clear();fail_on=2;assert(!sr_dynamics_convert(sink,nullptr,p,2,4101,0,&failed));
    assert(sink_calls==2 && !failed.active && written_l.size()==2048);
    assert(!sr_dynamics_convert(sink,nullptr,p,2,4101,0,&failed) && sink_calls==2);
    sr_session invalid{};assert(!sr_start_processing(&invalid,{44100,2,24,0},{0,1}));
    l.back()=std::numeric_limits<float>::quiet_NaN();clear();exact=true;
    assert(sr_dynamics_convert(sink,nullptr,p,2,4101,0,&invalid) && sink_calls==1 && invalid.frames==0);
    clear();exact=true;assert(sr_dynamics_convert(sink,nullptr,p,2,4101,0,nullptr) && sink_calls==1);
    sr::DynamicsSessions race;
    std::atomic<bool> run{true};
    std::thread worker([&]{while(run.load()) {auto lease=race.acquire(500);if(lease){assert(lease.engine()->active);lease.engine()->agc_db=.5;}}});
    for(int i=0;i<10000;i++){auto token=race.begin(500,{0,1});assert(token.generation);race.retire(token);}
    run.store(false);worker.join();
    std::puts("dynamics candidate: explicit generations, same-worker continuity, retirement, reuse, bounded capacity, private planes, stereo ceiling, subblock invariance, failed sink and nonfinite fallback passed");
}
