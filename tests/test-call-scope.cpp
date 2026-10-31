/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-call-scope.hpp"
#include <cassert>
#include <thread>
#include <stdexcept>
#include <cstdio>
int main() {
    using sr::CallScope;
    assert(!CallScope::permits(2));
    {
        CallScope outer(1,2);assert(CallScope::permits(2));
        assert(!CallScope::permits(3));
        {
            CallScope::Conversion busy(2);assert(bool(busy));
            CallScope::Conversion recurse(2);assert(!bool(recurse));
        }
        assert(CallScope::permits(2));
        {
            CallScope nested(4,5);
            assert(!CallScope::permits(2) && !CallScope::permits(5));
        }
        assert(CallScope::permits(2));
        std::thread t([]{
            assert(!CallScope::permits(2));
            CallScope own(6,7);assert(CallScope::permits(7));
            assert(!CallScope::permits(2));
        });t.join();
        try {
            CallScope::Conversion busy(2);assert(bool(busy));
            throw std::runtime_error("fixture");
        } catch(const std::runtime_error&) {}
        assert(CallScope::permits(2));
    }
    assert(!CallScope::permits(2));
    try {CallScope scope(1,2);throw 1;}catch(int){}
    assert(!CallScope::permits(2));
    {CallScope invalid(0,2);assert(!CallScope::permits(2));}
    std::puts("call scope: thread isolation, exact identity, nested/reentrant bypass and unwind restore passed");
}
