/*****************************************************************
 *  util.h                                                        *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#ifndef CADENCE_UTIL_H
#define CADENCE_UTIL_H

#include "context.h"

// Helper for for loops :)
#define fori(lim) for(int i = 0; i < (lim); i++)

float mtof(int midi);

float lerp(float a, float b, float t);
float clamp(float min, float max, float x);

float rand_float(float min, float max);
int rand_int(int min, int max);


typedef struct line_t {
  float len_samples;
  float rem_samples;
  float start_val;
  float end_val;
} line_t;

void set_line(cadence_ctx* ctx, line_t* l, float len_secs, float start, float end);
void reset_line(line_t* l);
float line(line_t* l, int* done);


typedef struct adsr_t {
  line_t atk;
  line_t rel;
} adsr_t;

void reset_adsr(adsr_t* l);
float adsr(adsr_t* adsr, int trig_rel, int* done);

typedef struct point {
  float x;
  float y;
} point;

point bezier(point p1, point p2, float curvature, float t);

float mix(float a, float b, float mix);

#endif
