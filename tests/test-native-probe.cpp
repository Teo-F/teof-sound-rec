/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-native-probe.hpp"
#include "rec-native-probe-api.h"
#include "rec-call-scope.hpp"
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <cstdio>
struct Worker {unsigned char padding[24];void *writer;};
static_assert(offsetof(Worker,writer)==24);
static bool throwing;
static const float *const *opaque=reinterpret_cast<const float *const *>(std::uintptr_t(1));
static bool convert(void *writer,const float *const *p,int ch,int frames) {
    assert(writer==reinterpret_cast<void *>(std::uintptr_t(42)));
    assert(p==opaque && ch==2 && frames==731);
    if(throwing)throw std::runtime_error("native fixture");
    return false; /* false must remain false, no retry */
}
static int worker(void *w) {
    auto *obj=static_cast<Worker *>(w);
    assert(!sr_native_convert_probe(obj->writer,opaque,2,731));
    return 10; /* scheduler return propagated unchanged */
}
int main() {
    sr_native_probe_set_originals(worker,convert);
    sr_native_headroom_enable(1);assert(sr_native_headroom_set(0));
    Worker w={{0},reinterpret_cast<void *>(std::uintptr_t(42))};
    assert(sr_native_worker_probe(&w)==10);
    auto c=sr_native_probe_read();assert(c.worker_calls==1 && c.scoped_converts==1 && c.other_converts==0);
    assert(!sr_native_convert_probe(w.writer,opaque,2,731));
    c=sr_native_probe_read();assert(c.other_converts==1);
    throwing=true;
    try{sr_native_worker_probe(&w);assert(false);}catch(const std::runtime_error&){}
    assert(!sr::CallScope::permits(42));
    throwing=false;assert(sr_native_worker_probe(&w)==10);
    std::puts("native probe candidate: original argument/result forwarding, unreadable planes, scoped identity and unwind passed");
}
