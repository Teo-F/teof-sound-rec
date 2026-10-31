/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-native-probe.hpp"
#include "rec-call-scope.hpp"
#include "rec-native-probe-api.h"
#include <atomic>
#include <cstring>
#include <cstdio>
#ifdef SR_NATIVE_HEADROOM
#include "rec-headroom-copy.hpp"
#endif
#ifdef SR_NATIVE_LIFECYCLE
#include "rec-native-lifecycle.hpp"
#endif
#ifdef SR_NATIVE_DYNAMICS
#include "rec-dynamics-copy.hpp"
extern "C" int sr_native_dynamics_set(int flags){return sr_dynamics_set(flags);}
extern "C" int sr_native_dynamics_get(void){return sr_dynamics_get();}
extern "C" void sr_native_dynamics_status(char *out,unsigned cap){sr_dynamics_status(out,cap);}
#endif
static sr_native_worker_fn original_worker;
static sr_native_convert_fn original_convert;
static std::atomic<std::uint64_t> workers{0},scoped{0},others{0};
static std::atomic<int> requested_headroom{0};
static std::atomic<bool> headroom_enabled{false};
extern "C" void sr_native_headroom_enable(int on){headroom_enabled.store(on!=0,std::memory_order_release);}
extern "C" int sr_native_headroom_set(int db) {
    if(db!=0 && db!=3 && db!=6 && db!=9 && db!=12)return 0;
    requested_headroom.store(db,std::memory_order_release);return 1;
}
extern "C" int sr_native_headroom_get(void){return requested_headroom.load(std::memory_order_acquire);}
void sr_native_probe_set_originals(sr_native_worker_fn w,sr_native_convert_fn c) noexcept {
    __atomic_store_n(&original_worker,w,__ATOMIC_RELEASE);
    __atomic_store_n(&original_convert,c,__ATOMIC_RELEASE);
}
sr_native_probe_counts sr_native_probe_read() noexcept {
    return {workers.load(std::memory_order_relaxed),scoped.load(std::memory_order_relaxed),
            others.load(std::memory_order_relaxed)};
}
extern "C" int sr_native_worker_probe(void *worker) {
    auto fn=__atomic_load_n(&original_worker,__ATOMIC_ACQUIRE);
    if(!fn)return 10;
    if(!worker)return fn(worker);
    /* Native worker's valid original call supplies this live object. Do not
     * call with an arbitrary address. No pointer retained beyond this scope. */
    std::uintptr_t writer=0;
    std::memcpy(&writer,static_cast<const unsigned char *>(worker)+24,sizeof(writer));
    sr::CallScope scope(reinterpret_cast<std::uintptr_t>(worker),writer,sr_native_headroom_get());
#ifdef SR_NATIVE_LIFECYCLE
    auto lease=sr_native_sessions.acquire(reinterpret_cast<std::uintptr_t>(worker));
#ifdef SR_NATIVE_DYNAMICS
    sr_worker_dynamics_scope dynamics_scope(lease.engine(),lease.generation());
#endif
    if(sr_lifecycle_ready.load(std::memory_order_acquire))
        (lease ? sr_session_workers:sr_missing_workers).fetch_add(1,std::memory_order_relaxed);
#endif
    workers.fetch_add(1,std::memory_order_relaxed);
    return fn(worker);
}
extern "C" bool sr_native_convert_probe(void *writer,const float *const *planes,int channels,int frames) {
    auto fn=__atomic_load_n(&original_convert,__ATOMIC_ACQUIRE);
    if(!fn)return false;
    int headroom=sr::CallScope::headroom_db();
    sr::CallScope::Conversion guard(reinterpret_cast<std::uintptr_t>(writer));
    (guard ? scoped:others).fetch_add(1,std::memory_order_relaxed);
#ifdef SR_NATIVE_DYNAMICS
    if(guard && headroom_enabled.load(std::memory_order_acquire)) {
        if(sr_worker_session) {
            bool result=sr_dynamics_convert(fn,writer,planes,channels,frames,headroom,sr_worker_session);
            sr_dynamics_publish(*sr_worker_session,sr_worker_generation);
            sr_dynamics_calls.fetch_add(1,std::memory_order_relaxed);
            return result;
        }
        sr_dynamics_missing.fetch_add(1,std::memory_order_relaxed);
        /* Do not silently write unlimited PCM if an enabled processing
         * session cannot be exclusively leased. Native false result handles
         * this as a failed write; never pretend processing succeeded. */
        if(sr_current_dynamics_flags.load(std::memory_order_acquire)!=0)return false;
    }
#endif
#ifdef SR_NATIVE_HEADROOM
    if(guard && headroom && headroom_enabled.load(std::memory_order_acquire))
        return sr_headroom_convert(fn,writer,planes,channels,frames,headroom);
#else
    (void)headroom;
#endif
    /* At default0dB/outside REC forward EXACT original pointer/result. No plane
     * dereference, copying, chunking, format change or float processing. */
    return fn(writer,planes,channels,frames);
}
extern "C" int sr_native_probe_attach(const az_core_v1 *api,const char *id) {
    if(!api || !id || !AZ_HAS(api,claim_hook) || !AZ_HAS(api,fw_tag))return -1;
    const char *tag=api->fw_tag();
    if(!tag || std::strcmp(tag,"ep147-9e9454e357389ece"))return -2;
    constexpr std::uintptr_t worker_address=0x230c640,convert_address=0x23e9340;
    const std::uint32_t worker_words[]={0xa9b17bfd,0x52800042,0x910003fd,0xf9401401};
    const std::uint32_t convert_words[]={0xd2890c10,0xcb3063ff,0x7100005f,0xa9007bfd};
    if(std::memcmp(reinterpret_cast<const void *>(worker_address),worker_words,sizeof(worker_words)) ||
       std::memcmp(reinterpret_cast<const void *>(convert_address),convert_words,sizeof(convert_words)))return -3;
    /* SDK supplies each original trampoline before publishing that hook.
     * A failed later claim leaves only diagnostic forwarding, never processing. */
    if(api->claim_hook(id,worker_address,reinterpret_cast<void *>(sr_native_worker_probe),
                      reinterpret_cast<void **>(&original_worker)))return -4;
    if(api->claim_hook(id,convert_address,reinterpret_cast<void *>(sr_native_convert_probe),
                      reinterpret_cast<void **>(&original_convert)))return -5;
#ifdef SR_NATIVE_LIFECYCLE
    if(sr_native_lifecycle_attach(api,id))return -6;
#endif
    return 0;
}
extern "C" void sr_native_probe_tick(const az_core_v1 *api,const char *id) {
    static std::uint64_t seen=0;
    auto counts=sr_native_probe_read();
    if(counts.worker_calls>=seen+100 && AZ_HAS(api,log)) {
        seen=counts.worker_calls;
        api->log(id,"REC identity probe workers=%llu scoped=%llu other=%llu (REC scope counts)",
                 static_cast<unsigned long long>(counts.worker_calls),
                 static_cast<unsigned long long>(counts.scoped_converts),
                 static_cast<unsigned long long>(counts.other_converts));
    }
}
extern "C" void sr_native_probe_status(char *out,unsigned capacity) {
    if(!out || !capacity)return;
#ifdef SR_NATIVE_LIFECYCLE
    sr_lifecycle_status(out,capacity);return;
#endif
    auto c=sr_native_probe_read();
    std::snprintf(out,capacity,"Probe W:%llu matched:%llu other:%llu",
        static_cast<unsigned long long>(c.worker_calls),static_cast<unsigned long long>(c.scoped_converts),
        static_cast<unsigned long long>(c.other_converts));
}
