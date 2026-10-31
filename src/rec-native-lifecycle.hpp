/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_NATIVE_LIFECYCLE_HPP
#define REC_NATIVE_LIFECYCLE_HPP
#include "rec-dynamics-sessions.hpp"
#include "azmod-mod.h"
#ifdef SR_NATIVE_DYNAMICS
#include "rec-dynamics-control.hpp"
#endif
#include <atomic>
#include <cstring>
#include <cstdio>
using sr_native_construct_fn=std::uintptr_t (*)(std::uintptr_t,std::uintptr_t,std::uintptr_t,
    std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t);
using sr_native_destroy_fn=void (*)(void *);
inline sr_native_construct_fn sr_original_construct;
inline sr_native_destroy_fn sr_original_destroy;
inline sr::DynamicsSessions sr_native_sessions;
inline std::atomic<std::uint64_t> sr_constructs{0},sr_construct_failures{0},sr_destroys{0},
    sr_session_workers{0},sr_missing_workers{0};
inline std::atomic<bool> sr_lifecycle_ready{false};
/* Candidate raw machine ABI: entry consumes x0..x7 plus originalsp[0];
 * x5/x7 moved as raw bits into d8/d9 internally, no FP argument consumed.
 * Preserve all nine integer words and original x0 return exactly. */
extern "C" inline std::uintptr_t sr_native_construct_probe(std::uintptr_t a0,std::uintptr_t a1,
    std::uintptr_t a2,std::uintptr_t a3,std::uintptr_t a4,std::uintptr_t a5,
    std::uintptr_t a6,std::uintptr_t a7,std::uintptr_t a8) {
    auto fn=__atomic_load_n(&sr_original_construct,__ATOMIC_ACQUIRE);
    if(!fn)return 0;
    sr_constructs.fetch_add(1,std::memory_order_relaxed);
#ifdef SR_NATIVE_DYNAMICS
    int flags=sr_dynamics_get();
    sr_current_dynamics_flags.store(flags,std::memory_order_release);
    auto token=sr_native_sessions.begin(a0,{flags&1,(flags>>1)&1});
    sr_dynamics_generation.store(token.generation,std::memory_order_release);
    sr_dynamics_reading.store(0,std::memory_order_release);
    sr_dynamics_missing.store(0,std::memory_order_release);
#else
    auto token=sr_native_sessions.begin(a0,{0,0});
#endif
    struct Pending {
        sr::DynamicsSessions::Token token;bool done=false;
        ~Pending(){if(!done){
            sr_native_sessions.retire(token);sr_construct_failures.fetch_add(1);
#ifdef SR_NATIVE_DYNAMICS
            auto expected=token.generation;sr_dynamics_generation.compare_exchange_strong(expected,0);
#endif
        }}
    } pending{token};
    auto result=fn(a0,a1,a2,a3,a4,a5,a6,a7,a8);pending.done=true;return result;
}
extern "C" inline void sr_native_destroy_probe(void *worker) {
    auto fn=__atomic_load_n(&sr_original_destroy,__ATOMIC_ACQUIRE);if(!fn)return;
    auto token=sr_native_sessions.current(reinterpret_cast<std::uintptr_t>(worker));
    /* Retire before complete destructor original starts. Existing leases finish in their
     * own fixed slot; no native pointer dereference from retirement. */
    sr_native_sessions.retire(token);sr_destroys.fetch_add(1,std::memory_order_relaxed);
#ifdef SR_NATIVE_DYNAMICS
    auto expected=token.generation;sr_dynamics_generation.compare_exchange_strong(expected,0);
#endif
    fn(worker);
}
inline int sr_native_lifecycle_attach(const az_core_v1 *api,const char *id) {
    if(!AZ_HAS(api,claim_hook))return -1;
    constexpr std::uintptr_t ctor=0x230e390,dtor=0x2307910;
    const std::uint32_t cw[]={0xa9b97bfd,0x910003fd,0xa9025bf5,0xaa0003f6};
    const std::uint32_t dw[]={0xa9be7bfd,0xaa0003e1,0x90007802,0x91076042};
    if(std::memcmp(reinterpret_cast<const void *>(ctor),cw,sizeof(cw)) ||
       std::memcmp(reinterpret_cast<const void *>(dtor),dw,sizeof(dw)))return -2;
    if(api->claim_hook(id,ctor,reinterpret_cast<void *>(sr_native_construct_probe),
                      reinterpret_cast<void **>(&sr_original_construct)))return -3;
    if(api->claim_hook(id,dtor,reinterpret_cast<void *>(sr_native_destroy_probe),
                      reinterpret_cast<void **>(&sr_original_destroy)))return -4;
    sr_lifecycle_ready.store(true,std::memory_order_release);return 0;
}
inline void sr_lifecycle_status(char *out,unsigned capacity) {
    std::snprintf(out,capacity,"Life C:%llu D:%llu W:%llu missing:%llu fail:%llu",
      (unsigned long long)sr_constructs.load(),(unsigned long long)sr_destroys.load(),
      (unsigned long long)sr_session_workers.load(),(unsigned long long)sr_missing_workers.load(),
      (unsigned long long)sr_construct_failures.load());
}
#endif
