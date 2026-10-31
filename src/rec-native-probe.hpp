/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_NATIVE_PROBE_HPP
#define REC_NATIVE_PROBE_HPP
#include <cstdint>
using sr_native_worker_fn=int (*)(void *);
using sr_native_convert_fn=bool (*)(void *,const float *const *,int,int);
struct sr_native_probe_counts {std::uint64_t worker_calls,scoped_converts,other_converts;};
/* Preparatory exact-signature candidate, not an installed hook service.
 * Original callbacks set before any invocation; no audio samples changed/read. */
void sr_native_probe_set_originals(sr_native_worker_fn,sr_native_convert_fn) noexcept;
sr_native_probe_counts sr_native_probe_read() noexcept;
extern "C" int sr_native_worker_probe(void *worker);
extern "C" bool sr_native_convert_probe(void *writer,const float *const *planes,int channels,int frames);
#endif
