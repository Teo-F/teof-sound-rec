/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-native-probe.hpp"
#include "rec-native-probe-api.h"
#include "rec-native-lifecycle.hpp"
#include <cassert>
#include <cmath>
#include <vector>
#include <cstring>
#include <cstdio>
struct Worker {unsigned char padding[24];void *writer;};
static Worker w{};
static std::vector<float> l(4101,2.f),r(4101,.5f),out;
static const float *p[]={l.data(),r.data()};
static int calls,fail_on;static bool exact;
static bool sink(void *writer,const float *const *planes,int ch,int frames) {
    assert(writer==w.writer && ch==2);++calls;
    if(exact)assert(planes==p && frames==4101);else assert(planes!=p && frames<=2048);
    if(calls==fail_on)return false;
    out.insert(out.end(),planes[0],planes[0]+frames);return true;
}
static int worker(void *ptr){return sr_native_convert_probe(static_cast<Worker*>(ptr)->writer,p,2,4101) ? 7:10;}
static std::uintptr_t construct(std::uintptr_t a0,std::uintptr_t,std::uintptr_t,std::uintptr_t,
 std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t){assert(a0==reinterpret_cast<std::uintptr_t>(&w));return 123;}
static void destroy(void *ptr){assert(ptr==&w);}
static void begin(int flags){assert(sr_native_dynamics_set(flags));assert(sr_native_construct_probe(reinterpret_cast<std::uintptr_t>(&w),0,0,0,0,0,0,0,0)==123);calls=fail_on=0;out.clear();}
int main(){
    w.writer=reinterpret_cast<void*>(42);sr_native_probe_set_originals(worker,sink);
    sr_original_construct=construct;sr_original_destroy=destroy;
    assert(!sr_native_dynamics_set(1) && !sr_native_dynamics_set(4));
    sr_native_headroom_enable(1);sr_native_headroom_set(0);
    begin(0);exact=true;assert(sr_native_worker_probe(&w)==7 && calls==1 && out==l);
    assert(sr_native_dynamics_set(2));calls=0;out.clear(); // NextREC flags cannot affect current.
    assert(sr_native_worker_probe(&w)==7 && calls==1 && out==l);
    sr_native_destroy_probe(&w);
    begin(2);exact=false;assert(sr_native_worker_probe(&w)==7 && calls==3);
    for(float v:out)assert(std::abs(v)<=.891251);
    char status[160];sr_native_dynamics_status(status,sizeof(status));assert(std::strstr(status,"AGC:OFF LIM:ON") && std::strstr(status,"GR:7.0dB"));
    auto token=sr_native_sessions.current(reinterpret_cast<std::uintptr_t>(&w));
    {auto lease=sr_native_sessions.acquire(reinterpret_cast<std::uintptr_t>(&w));assert(lease && lease.engine()->frames==4101);}
    w.writer=reinterpret_cast<void*>(43);calls=0;out.clear();
    assert(sr_native_worker_probe(&w)==7); // Split/new writer preserves same worker engine.
    {auto lease=sr_native_sessions.acquire(reinterpret_cast<std::uintptr_t>(&w));assert(lease.engine()->frames==8202);}
    assert(sr_native_sessions.current(reinterpret_cast<std::uintptr_t>(&w)).generation==token.generation);
    sr_native_destroy_probe(&w);sr_native_dynamics_status(status,sizeof(status));assert(std::strstr(status,"waiting"));
    begin(3);assert(sr_native_dynamics_get()==3);exact=false;
    assert(sr_native_worker_probe(&w)==7);
    {auto lease=sr_native_sessions.acquire(reinterpret_cast<std::uintptr_t>(&w));assert(lease.engine()->agc_db<0 && lease.engine()->processing.limiter);}
    sr_native_destroy_probe(&w);
    begin(3);{auto lease=sr_native_sessions.acquire(reinterpret_cast<std::uintptr_t>(&w));assert(lease.engine()->agc_db==0 && lease.engine()->limiter_gain==1);}
    fail_on=2;assert(sr_native_worker_probe(&w)==10 && calls==2);
    assert(sr_native_worker_probe(&w)==10 && calls==2); // Failed sink latched, never retry.
    sr_native_dynamics_status(status,sizeof(status));assert(std::strstr(status,"FAILED"));
    sr_native_destroy_probe(&w);
    // A missing state lease with required dynamics must fail without a sink write.
    calls=0;assert(sr_native_worker_probe(&w)==10 && calls==0);
    // Outside the authenticated scope, even enabled dynamics do not read input.
    exact=true;calls=0;out.clear();fail_on=0;assert(sr_native_convert_probe(w.writer,p,2,4101) && calls==1 && out==l);
    puts("native dynamics: exact bypass, next-REC frozen settings, limiter route, telemetry, writer-split continuity, AGC reset and sink-failure latch passed");
}
