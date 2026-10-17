// Minimal subset of Soundpipe's Soundpipe.h - just what zitarev.c needs.
// Soundpipe is MIT licensed (see LICENSE); zitarev.c is copied unmodified apart from
// the block-compute entry point added at the end of that file.

#ifndef SOUNDPIPE_H
#define SOUNDPIPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef SPFLOAT
#define SPFLOAT float
#endif
#define SP_OK 1
#define SP_NOT_OK 0

typedef struct sp_data {
    SPFLOAT *out;
    int sr;
    int nchan;
    unsigned long len;
    unsigned long pos;
    char filename[200];
    uint32_t rand;
} sp_data;

typedef struct {
    void *faust;
    int argpos;
    SPFLOAT *args[11];
    SPFLOAT *in_delay;
    SPFLOAT *lf_x;
    SPFLOAT *rt60_low;
    SPFLOAT *rt60_mid;
    SPFLOAT *hf_damping;
    SPFLOAT *eq1_freq;
    SPFLOAT *eq1_level;
    SPFLOAT *eq2_freq;
    SPFLOAT *eq2_level;
    SPFLOAT *mix;
    SPFLOAT *level;
} sp_zitarev;

int sp_zitarev_create(sp_zitarev **p);
int sp_zitarev_destroy(sp_zitarev **p);
int sp_zitarev_init(sp_data *sp, sp_zitarev *p);
int sp_zitarev_compute(sp_data *sp, sp_zitarev *p, SPFLOAT *in1, SPFLOAT *in2, SPFLOAT *out1, SPFLOAT *out2);
/// Processes `count` frames in one call (the stock compute does one frame per call).
int sp_zitarev_compute_block(sp_zitarev *p, int count, SPFLOAT *in1, SPFLOAT *in2, SPFLOAT *out1, SPFLOAT *out2);

#ifdef __cplusplus
}
#endif

#endif
