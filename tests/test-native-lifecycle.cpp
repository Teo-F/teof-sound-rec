/* SPDX-License-Identifier: GPL-3.0-only */
#include "rec-native-lifecycle.hpp"
#include <cassert>
#include <stdexcept>
#include <cstdio>
static bool throwing;
static int deletes;
static std::uintptr_t construct(std::uintptr_t a0,std::uintptr_t a1,std::uintptr_t a2,
 std::uintptr_t a3,std::uintptr_t a4,std::uintptr_t a5,std::uintptr_t a6,
 std::uintptr_t a7,std::uintptr_t a8) {
 assert(a0==42 && a1==11 && a2==22 && a3==33 && a4==44 && a5==55 && a6==66 && a7==77 && a8==88);
 auto during=sr_native_sessions.acquire(42);assert(during && during.engine()->agc_db==0);
 if(throwing)throw std::runtime_error("constructor failure");
 return 0xfedcba9876543210ULL;
}
static void destroy(void *p){assert(reinterpret_cast<std::uintptr_t>(p)==42);assert(!sr_native_sessions.acquire(42));++deletes;}
int main(){
 sr_original_construct=construct;sr_original_destroy=destroy;
 auto invoke=[](){return sr_native_construct_probe(42,11,22,33,44,55,66,77,88);};
 assert(invoke()==0xfedcba9876543210ULL);
 auto first=sr_native_sessions.current(42);assert(first.generation);
 {auto lease=sr_native_sessions.acquire(42);assert(lease);lease.engine()->agc_db=5;}
 sr_native_destroy_probe(reinterpret_cast<void *>(42));assert(deletes==1 && !sr_native_sessions.acquire(42));
 throwing=true;try {invoke();assert(false);}catch(const std::runtime_error&){}
 assert(!sr_native_sessions.acquire(42) && sr_construct_failures.load()==1);
 throwing=false;assert(invoke()==0xfedcba9876543210ULL);
 auto fresh=sr_native_sessions.current(42);assert(fresh.generation!=first.generation);
 {auto lease=sr_native_sessions.acquire(42);assert(lease.engine()->agc_db==0);}
 sr_native_destroy_probe(reinterpret_cast<void *>(42));assert(deletes==2);
 puts("lifecycle candidate: nine raw argument words, return forwarding, constructor-time lease, failure unwind, deletion retirement and same-address new generation passed");
}
