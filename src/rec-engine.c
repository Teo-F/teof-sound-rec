/* SPDX-License-Identifier: GPL-3.0-only */
/* Portable processing contract; NOT bound to EP147 or its threads. */
#include "rec-engine.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <float.h>
int sr_format_valid(sr_format f) {
    return f.rate == 44100 && f.channels == 2 &&
        (f.bits == 16 || f.bits == 24) && f.headroom_db >= 0 &&
        f.headroom_db <= 12 && f.headroom_db % 3 == 0;
}
int sr_start(sr_session *s, sr_format actual) {
    return sr_start_processing(s, actual, (sr_processing){0,0});
}
int sr_start_processing(sr_session *s, sr_format actual, sr_processing processing) {
    if (!s || s->active || !sr_format_valid(actual)) return -1;
    if ((processing.auto_gain != 0 && processing.auto_gain != 1) ||
        (processing.limiter != 0 && processing.limiter != 1)) return -1;
    *s = (sr_session){.active=1, .format=actual, .processing=processing,
                     .limiter_gain=1.};
    return 0;
}
/* Inputs and outputs must be separate, nonoverlapping stereo planes. Reject
 * nonfinite samples before any output/state mutation. Caller owns all buffers. */
static int overlap(const void *a, const void *b, size_t n) {
    uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x <= y ? y-x < n : x-y < n;
}
int sr_process(sr_session *s, const float *l, const float *r,
               float *ol, float *or_, size_t n) {
    if (!s || !s->active || !sr_format_valid(s->format)) return -1;
    if (!n) return 0;
    if (!l || !r || !ol || !or_ || n > SR_MAX_BLOCK || n > SIZE_MAX/sizeof(float) ||
        n > UINT64_MAX-s->frames || n > (UINT64_MAX-s->clips)/2) return -1;
    size_t bytes=n*sizeof(float);
    if (overlap(ol,or_,bytes) || overlap(ol,l,bytes) || overlap(ol,r,bytes) ||
        overlap(or_,l,bytes) || overlap(or_,r,bytes)) return -1;
    for (size_t i=0;i<n;i++) if (!isfinite(l[i]) || !isfinite(r[i])) return -2;
    static const double gains[]={1.,0.7079457843841379,0.5011872336272722,
                                0.35481338923357547,0.251188643150958};
    double gain=gains[s->format.headroom_db/3];
    if (s->processing.auto_gain && !s->processing.limiter) {
        double bound=FLT_MAX/(gain*1.99526231496888); /* max +6dB */
        for(size_t i=0;i<n;i++)
            if(fabs(l[i])>bound || fabs(r[i])>bound) return -3;
    }
    const double power_alpha=1.-exp(-1./(.4*s->format.rate));
    const double release=exp(-1./(.1*s->format.rate));
    /* Sample-peak ceiling -1 dBFS, conservatively rounded below the target.
     * NOT a true-peak/lookahead limiter. No added delay. */
    const double ceiling=(double)nextafterf((float)0.8912509381337456,0.f);
    for (size_t i=0;i<n;i++) {
        float a=l[i], b=r[i];
        double ag=1.;
        if (s->processing.auto_gain) {
            double power=((double)a*a+(double)b*b)*.5;
            s->rms_power+=power_alpha*(power-s->rms_power);
            /* Gate instantaneous AND smoothed power: silence never raises gain.
             * Target -18dBFS RMS, gain bounds -18..+6dB, conservative slew. */
            if (power>1e-6 && s->rms_power>1e-6) {
                double target=fmax(-18.,fmin(6.,-18.-10.*log10(s->rms_power)));
                double delta=target-s->agc_db;
                double step=(delta<0 ? 3. : .5)/s->format.rate;
                s->agc_db+=fmax(-step,fmin(step,delta));
            }
            ag=pow(10.,s->agc_db/20.);
        }
        double x=(double)a*gain*ag, y=(double)b*gain*ag;
        double lg=1.;
        if (s->processing.limiter) {
            double peak=fmax(fabs(x),fabs(y));
            double required=peak>ceiling ? ceiling/peak : 1.;
            double relaxed=1.-(1.-s->limiter_gain)*release;
            lg=fmin(required,relaxed); /* instantaneous linked attack */
            s->limiter_gain=lg;
            s->reduction_db=-20.*log10(fmax(lg,1e-300));
        }
        /* OFF preserves original bits including signed zero. */
        int bypass= gain==1. && !s->processing.auto_gain && !s->processing.limiter;
        ol[i]=bypass ? a : (float)(x*lg);
        or_[i]=bypass ? b : (float)(y*lg);
        double pi=fmax(fabs(a),fabs(b));
        double po=fmax(fabs(ol[i]),fabs(or_[i]));
        s->peak_in=fmax(s->peak_in,pi); s->peak_out=fmax(s->peak_out,po);
        /* Samples at or above digital full scale, not proof of prior clipping. */
        s->clips += (fabs(a)>=1.) + (fabs(b)>=1.);
    }
    s->frames+=n;
    return 0;
}
void sr_stop(sr_session *s) { if (s) s->active=0; }
double sr_elapsed(const sr_session *s) {
    return s && sr_format_valid(s->format) ? (double)s->frames/s->format.rate : -1.;
}
uint64_t sr_space_seconds(sr_format f, uint64_t available, uint64_t reserve) {
    if (!sr_format_valid(f) || available<=reserve) return 0;
    uint64_t bytes_sec=(uint64_t)f.rate*f.channels*(f.bits/8);
    return (available-reserve)/bytes_sec;
}
int sr_prefix_valid(const char *p) {
    if (!p) return 0;
    size_t n=0;
    while (*p) {
        unsigned char c=(unsigned char)*p++;
        if (++n>24 || !((c>='A'&&c<='Z') || (c>='a'&&c<='z') ||
                       (c>='0'&&c<='9') || c=='_' || c=='-')) return 0;
    }
    return 1; /* empty means native naming */
}
