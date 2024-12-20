#ifndef CADENCE_FFT_H
#define CADENCE_FFT_H

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <complex.h>

#define i16 int16_t
#define i64 int64_t
#define f32 float

typedef enum fft_stages {
  FIRST_ITERATION,
  FIRST_ITERATION_DONE,
} fft_stages;

typedef struct fft_t {
  size_t size;

  fft_stages stage;
  int samples_ready;
  
  int  sample_index; // since we receive one sample at a time, we need keep track of when buffers are full
  f32* in_buf;

  f32* overlap_buf; // Used for overlap storage
  f32 complex* buf;
  f32 complex* pers; // persistent fft, is a copy of the fft'd signal. persists when fft is transformed back
} fft_t;



void  new_fft(fft_t* obj, int size);
void  apply_fft(fft_t* obj, float sample);
float apply_ifft(fft_t* obj);

void spectral_shift(fft_t* fft, float factor);

#endif
