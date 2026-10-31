/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef REC_NATIVE_PROBE_API_H
#define REC_NATIVE_PROBE_API_H
#include "azmod-mod.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Diagnostic only; no processing controls enabled by these calls. */
int sr_native_probe_attach(const az_core_v1 *api,const char *id);
void sr_native_probe_tick(const az_core_v1 *api,const char *id);
void sr_native_probe_status(char *out,unsigned capacity);
int sr_native_headroom_set(int attenuation_db);
int sr_native_headroom_get(void);
void sr_native_headroom_enable(int enabled);
#ifdef SR_NATIVE_DYNAMICS
int sr_native_dynamics_set(int flags); /* 0 OFF,2 limiter,3 AGC+limiter */
int sr_native_dynamics_get(void);
void sr_native_dynamics_status(char *out,unsigned capacity);
#endif
#ifdef __cplusplus
}
#endif
#endif
