/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_CALL_SCOPE_HPP
#define REC_CALL_SCOPE_HPP
#include <cstdint>
namespace sr {
/* Candidate call-scoped identity gate. Does NOT authenticate native events,
 * lease objects or bind an ABI. Only integer identities, no dereference. */
class CallScope {
    inline static thread_local CallScope *current_=nullptr;
    CallScope *previous_;
    std::uintptr_t worker_,writer_;
    bool converting_=false;
    int headroom_db_;
public:
    CallScope(std::uintptr_t worker,std::uintptr_t writer,int headroom_db=0) noexcept:
        previous_(current_),worker_(worker),writer_(writer),headroom_db_(headroom_db) {current_=this;}
    ~CallScope() noexcept {current_=previous_;}
    CallScope(const CallScope&)=delete;
    CallScope& operator=(const CallScope&)=delete;
    static bool permits(std::uintptr_t writer) noexcept {
        auto *s=current_;
        return s && !s->previous_ && s->worker_ && s->writer_ &&
               s->writer_==writer && !s->converting_;
    }
    static int headroom_db() noexcept {return current_ ? current_->headroom_db_:0;}
    class Conversion {
        CallScope *owner_=nullptr;
    public:
        explicit Conversion(std::uintptr_t writer) noexcept {
            if(permits(writer)){owner_=current_;owner_->converting_=true;}
        }
        ~Conversion() noexcept {if(owner_)owner_->converting_=false;}
        explicit operator bool() const noexcept {return owner_!=nullptr;}
        Conversion(const Conversion&)=delete;
        Conversion& operator=(const Conversion&)=delete;
    };
};
}
#endif
