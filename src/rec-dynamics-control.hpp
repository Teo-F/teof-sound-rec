/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_DYNAMICS_CONTROL_HPP
#define REC_DYNAMICS_CONTROL_HPP
#include <atomic>
#include <cstdint>
#include <cmath>
#include <cstdio>
inline std::atomic<int> sr_next_dynamics{0},sr_current_dynamics_flags{0};
inline std::atomic<std::uint64_t> sr_dynamics_generation{0},sr_dynamics_reading{0};
inline std::atomic<std::uint64_t> sr_dynamics_calls{0},sr_dynamics_missing{0};
inline thread_local sr_session *sr_worker_session=nullptr;
inline thread_local std::uint64_t sr_worker_generation=0;
inline int sr_dynamics_set(int flags) {
    /* AutoGain requires sample-peak limiting. No unsupported AGC-only mode. */
    if(flags!=0 && flags!=2 && flags!=3)return 0;
    sr_next_dynamics.store(flags,std::memory_order_release);return 1;
}
inline int sr_dynamics_get(void){return sr_next_dynamics.load(std::memory_order_acquire);}
inline void sr_dynamics_publish(const sr_session &s,std::uint64_t generation) {
    if(!generation || sr_dynamics_generation.load(std::memory_order_acquire)!=generation)return;
    int gain=(int)std::lround(s.agc_db*10.0)+180;
    int reduction=(int)std::lround(s.reduction_db*10.0);
    if(gain<0)gain=0;
    if(gain>240)gain=240;
    if(reduction<0)reduction=0;
    if(reduction>4095)reduction=4095;
    unsigned flags=(s.processing.auto_gain ? 1u:0u)|(s.processing.limiter ? 2u:0u);
    std::uint32_t payload=1u|(flags<<1)|((unsigned)gain<<3)|((unsigned)reduction<<11)|(!s.active ? (1u<<23):0);
    sr_dynamics_reading.store((generation<<32)|payload,std::memory_order_release);
}
inline void sr_dynamics_status(char *out,unsigned capacity) {
    if(!out || !capacity)return;
    auto generation=sr_dynamics_generation.load(std::memory_order_acquire);
    auto value=sr_dynamics_reading.load(std::memory_order_acquire);
    if(sr_dynamics_missing.load(std::memory_order_relaxed)) {
        std::snprintf(out,capacity,"REC processing state unavailable; check test log");return;
    }
    if(!generation || std::uint32_t(value>>32)!=std::uint32_t(generation) || !(value&1)) {
        std::snprintf(out,capacity,"REC processing: waiting for next REC samples");return;
    }
    unsigned v=(unsigned)value,flags=(v>>1)&3;
    double gain=(double((v>>3)&255)-180)/10.0,gr=double((v>>11)&4095)/10.0;
    std::snprintf(out,capacity,"Active AGC:%s LIM:%s | Gain:%+.1fdB GR:%.1fdB%s",
       flags&1 ? "ON":"OFF",flags&2 ? "ON":"OFF",gain,gr,v&(1u<<23) ? " FAILED":"");
}
class sr_worker_dynamics_scope {
    sr_session *previous_;std::uint64_t generation_;
public:
    sr_worker_dynamics_scope(sr_session *session,std::uint64_t generation):
        previous_(sr_worker_session),generation_(sr_worker_generation) {
        sr_worker_session=session;sr_worker_generation=generation;
    }
    ~sr_worker_dynamics_scope(){sr_worker_session=previous_;sr_worker_generation=generation_;}
};
#endif
