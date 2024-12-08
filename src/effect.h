/*****************************************************************
 *  effect.h                                                      *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#ifndef CADENCE_EFFECT_H
#define CADENCE_EFFECT_H

#include "context.h"
#include "reverb.h"

typedef struct butlp_t {
  float cutoff_freq;
  float a0, a1, a2;
  float b0, b1, b2;
  float x1, x2;  // Past input samples
  float y1, y2;  // Past output samples
} butlp_t;

butlp_t* new_butlp(cadence_ctx* ctx, float freq);
float    apply_butlp(cadence_ctx* ctx, butlp_t *filter, float input, float cutoff_freq);

typedef struct delay_t {
  float*   buffer;
  uint32_t buf_size;
  uint32_t write_head;
  float read_offset;
  float last_offset;
} delay_t;
delay_t* new_delay(cadence_ctx* ctx, int samples);
float    apply_delay(struct cadence_ctx* ctx, delay_t* d, float sample, float delay_ms, float feedback);

typedef struct reverb_t {
  reverbBlock rb;
  float chunk[32];
  int chunk_idx; // Used to keep track of filling the chunk
} reverb_t;
reverb_t* new_reverb(cadence_ctx* ctx);
void      set_reverb(cadence_ctx* ctx, reverb_t *r, float wet_percent, float time_s, float room_size_s,
		     float cutoff_hz, float pre_delay_s);
float     apply_reverb(cadence_ctx *ctx, reverb_t* r, float input);

#endif
