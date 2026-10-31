/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_DYNAMICS_SESSIONS_HPP
#define REC_DYNAMICS_SESSIONS_HPP
extern "C" {
#include "rec-engine.h"
}
#include <mutex>
#include <atomic>
#include <cstdint>
#include <array>
namespace sr {
/* Native integration candidate: explicit lifecycle generations, no object
 * dereference. Fixed storage; worker acquire/release never locks or allocates.
 * Lifecycle begin/retire serialize with each other, not with the worker. */
class DynamicsSessions {
    enum State {Free,Building,Idle,Busy,RetiringBusy};
    struct Slot {
        std::atomic<int> state{Free};
        std::atomic<std::uintptr_t> identity{0};
        std::atomic<std::uint64_t> generation{0};
        sr_session engine{};
    };
    std::array<Slot,8> slots_{};
    std::mutex lifecycle_;
    std::uint64_t generation_=0;
    static void clear(Slot &s) {
        s.engine={};s.identity.store(0,std::memory_order_relaxed);
        s.generation.store(0,std::memory_order_relaxed);
        s.state.store(Free,std::memory_order_release);
    }
    static void retire_slot(Slot &s) {
        int state=s.state.load(std::memory_order_acquire);
        while(state==Idle || state==Busy) {
            int target=state==Idle ? Building:RetiringBusy;
            if(s.state.compare_exchange_weak(state,target,std::memory_order_acq_rel)) {
                if(target==Building)clear(s);
                return;
            }
        }
    }
public:
    struct Token { std::uintptr_t identity=0;std::uint64_t generation=0; };
    class Lease {
        friend class DynamicsSessions;
        Slot *slot_=nullptr;
        explicit Lease(Slot *s):slot_(s){}
    public:
        Lease()=default;
        Lease(const Lease&)=delete;
        Lease& operator=(const Lease&)=delete;
        Lease(Lease&& x) noexcept:slot_(x.slot_){x.slot_=nullptr;}
        ~Lease(){if(slot_)release(slot_);}
        explicit operator bool()const{return slot_!=nullptr;}
        sr_session *engine(){return slot_ ? &slot_->engine:nullptr;}
        std::uint64_t generation()const{return slot_ ? slot_->generation.load(std::memory_order_relaxed):0;}
    };
    Token begin(std::uintptr_t identity,sr_processing settings) {
        if(!identity || settings.auto_gain<0 || settings.auto_gain>1 ||
           settings.limiter<0 || settings.limiter>1)return {};
        std::lock_guard<std::mutex> lock(lifecycle_);
        for(auto &s:slots_)if(s.identity.load(std::memory_order_relaxed)==identity)retire_slot(s);
        for(auto &s:slots_) {
            int free=Free;
            if(!s.state.compare_exchange_strong(free,Building,std::memory_order_acq_rel))continue;
            auto generation=++generation_;if(!generation)generation=++generation_;
            s.identity.store(identity,std::memory_order_relaxed);s.generation.store(generation,std::memory_order_relaxed);
            s.engine={};
            if(sr_start_processing(&s.engine,{44100,2,24,0},settings)){clear(s);return {};}
            s.state.store(Idle,std::memory_order_release);return {identity,generation};
        }
        return {};
    }
    Token current(std::uintptr_t identity) {
        std::lock_guard<std::mutex> lock(lifecycle_);
        for(auto &s:slots_) {
            int state=s.state.load(std::memory_order_acquire);
            if((state==Idle || state==Busy) && s.identity.load(std::memory_order_relaxed)==identity)
                return {identity,s.generation.load(std::memory_order_relaxed)};
        }
        return {};
    }
    void retire(Token token) {
        std::lock_guard<std::mutex> lock(lifecycle_);
        for(auto &s:slots_)if(s.identity.load(std::memory_order_relaxed)==token.identity &&
                             s.generation.load(std::memory_order_relaxed)==token.generation)retire_slot(s);
    }
    Lease acquire(std::uintptr_t identity) {
        for(auto &s:slots_) {
            if(s.identity.load(std::memory_order_relaxed)!=identity)continue;
            int idle=Idle;
            if(!s.state.compare_exchange_strong(idle,Busy,std::memory_order_acq_rel))continue;
            if(s.identity.load(std::memory_order_relaxed)==identity)return Lease(&s);
            release(&s);
        }
        return {};
    }
private:
    static void release(Slot *s) {
        int busy=Busy;
        if(s->state.compare_exchange_strong(busy,Idle,std::memory_order_release))return;
        /* Retirement owns no access to engine while this lease is in flight. */
        if(busy==RetiringBusy)clear(*s);
    }
};
}
#endif
