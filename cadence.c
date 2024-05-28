
/*
  Cadence Audio Engine
  May 2024
  Børge Lundsaunet
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <time.h>

#include "synth.h"

typedef struct audio_info {
  int rc;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int sample_rate;
  int dir;
  snd_pcm_uframes_t frames;
  int16_t *buffer;
  uint32_t buffer_size;
} audio_info;


#define num_tracks 4
#define track_size 256
#define channels_pr_track 2

typedef struct audio {
  audio_info info;
  float tracks[num_tracks][channels_pr_track*track_size];
} audio;


void audio_setup(audio** a);
void audio_cleanup(audio_info* a);
void audio_play_buffer(audio_info* a);


// declare global audio a so we don't have to pass it around :)
audio* a;

void write_to_track(int n, int i, float sample) {
  a->tracks[n][2*i]   = sample;
  a->tracks[n][2*i+1] = sample;
}

// -- sine --
typedef struct sine {
  double t;
  float freq;
} sine;

sine* new_sine() {
  sine* s = (sine*)malloc(sizeof(sine)); // @NOTE - hardcoded max buffer size
  s->t = 0;
  s->freq = 0;
  return s;
}

float gen_sine(sine* sine) {
  float   wave_period = a->info.sample_rate / sine->freq;
  float   sample      = sin(sine->t);

  sine->t += 2.0f * M_PI * 1.0f / wave_period;

  sine->t = fmod(sine->t, 2.0f*M_PI);
  return sample;
}


// -- phasor --
typedef struct phasor {
  double value;
  float freq;
} phasor;

phasor* new_phasor() {
  phasor* p = (phasor*)malloc(sizeof(phasor)); // @NOTE - hardcoded max buffer size
  p->value = 0;
  p->freq = 0;
  return p;
}

float gen_phasor(phasor* p) {
  double diff =  (double)p->freq / (double)a->info.sample_rate;
  p->value += diff;
  if (p->value > 1.0f) p->value -= 1.0f;
  return p->value;
}

// -- delay --
typedef struct delay {
  float*   buffer;
  uint32_t buf_size;
  uint32_t write_head;
  uint32_t read_offset;
} delay;

delay* new_delay() {
  delay* d = (delay*)malloc(sizeof(delay)); // @NOTE - hardcoded max buffer size
  d->buf_size = 10*a->info.sample_rate;
  d->buffer = malloc(d->buf_size * sizeof(float));

  // clear out buffer @NOTE - might be unecessary
  for (int i = 0; i < d->buf_size; i++) {
    d->buffer[i] = 0;
  }
  return d;
}

float apply_delay(delay* d, float sample, float delay_ms, float feedback) {
  d->read_offset = delay_ms * a->info.sample_rate; // @NOTE - hardcoded samplerate
  // read from delay buffer
  uint32_t index = (d->write_head - d->read_offset) % d->buf_size;
  float delayed  = d->buffer[index];

  // write back + feedback!
  d->buffer[d->write_head % d->buf_size] = sample + delayed * feedback;

  // increment delay
  d->write_head++;
  return delayed;
}

typedef enum gen_type {
  GEN_FREE,
  GEN_SINE,
  GEN_PHASOR,
} gen_type;

typedef struct gen_table {
  float val;
  union {
    sine* s;
    phasor* p;
  };
  gen_type type;
  int free;
} gen_table;

// @NOTE - I tried making to parallel arrays for processing but had no difference :)
// might try this again if I move the generator directly into the gen table
// instead of with a pointer reference
#define gen_table_size 64
gen_table gt[gen_table_size];

int register_gen_table(gen_table* gt, gen_type type) {
  fori(gen_table_size) {
    if (gt[i].type == GEN_FREE) {
      gt[i].type = type;
      switch(type) {
      case GEN_SINE: {
	gt[i].s = new_sine();
      } break;
      case GEN_PHASOR: {
	gt[i].p = new_phasor();
      } break;
      default: assert(0);
      }
      return i;
    }
  }
  return -1;
}

void del_gen_table(gen_table* gt, int i) {
  gt[i].type = GEN_FREE;
}

void process_gen_table(gen_table* gt) {
  fori(gen_table_size) {
    gen_type t = gt[i].type;
    switch(t) {
    case GEN_FREE: break;
    case GEN_SINE: {
      gt[i].val = gen_sine(gt[i].s);
    } break;
    case GEN_PHASOR: {
      gt[i].val = gen_phasor(gt[i].p);
    } break;
    }
  }
}


float test_osc(synth* s, float freq, float amp, int index, int* reset) {
  static sine** sines;
  static phasor** phasors;;
  static int init = 0;
  static int sine_i;

  if (!init) {
    init = 1;

    // set up local oscillators
    sines   = malloc(s->poly_count * sizeof(sine));
    phasors = malloc(s->poly_count * sizeof(phasor));
    fori(4) sines[i] = new_sine();
    fori(4) phasors[i] = new_phasor();

    // and global ones :)
    sine_i = register_gen_table(gt, GEN_SINE);
    gt[sine_i].p->freq = 0.8;
  }

  if (*reset) {
    sines[index]->t = 0;
    phasors[index]->value = 0;
    *reset = 0;
  }

  phasors[index]->freq = 1.0f;
  float phase = gen_phasor(phasors[index]);
  amp *= 1.0f-phase;

  float mod = gt[sine_i].val;

  sines[index]->freq = freq + 15.0*mod;
  float sample = amp * gen_sine(sines[index]);
  return sample;
}

int main(int argc, char** argv) {

  int perf_mode = 0;
  if (argc == 2) {
    printf("argv: %s\n", argv[1]);
    perf_mode = 1;
  }

  if (!perf_mode) {
    audio_setup(&a);
  } else {
    a = malloc(sizeof(audio));
    a->info.sample_rate = 44100;
    a->info.frames = 256;
    a->info.buffer = malloc(512*2*sizeof(int16_t));
  }

  fori(gen_table_size) gt[i].type = GEN_FREE;

  delay* d1 = new_delay();
  synth* s  = new_synth(8, test_osc);

  int n1;
  int n2;
  int n3;
  for (int j = 0; j < 1024; j++) {
    if (j == 0)   synth_register_note(s, 440.0f, 0.1, NOTE_ON, &n1);
    if (j == 100/2) synth_register_note(s, 660.0f, 0.1, NOTE_ON, &n2);
    if (j == 200/2) synth_register_note(s, 880.0f, 0.1, NOTE_ON, &n3);
    if (j == 600/2) synth_register_note(s, 440.0f, 0.1, NOTE_OFF, &n1);
    if (j == 700/2) synth_register_note(s, 660.0f, 0.1, NOTE_OFF, &n2);
    if (j == 800/2) synth_register_note(s, 880.0f, 0.1, NOTE_OFF, &n3);

    // process frames
    fori(a->info.frames) {

      // first handle global generators
      process_gen_table(gt);

      float sample  = synth_play(s);
      float delayed = apply_delay(d1, sample, 0.3, 0.6);
      write_to_track(0, i, sample + delayed);
    }

    // mix tracks
    // for every sample in each frame..
    fori(a->info.frames) {
      float sample_l = 0;
      float sample_r = 0;

      // ...sum up each channel :)
      for (int n = 0; n < num_tracks; n++) {
	sample_l += a->tracks[n][2*i];
	sample_r += a->tracks[n][2*i+1];
      }

      // sample to 16 bit int and copy to alsa buffer
      a->info.buffer[2*i]   = (int16_t)(32768.0f * sample_l);
      a->info.buffer[2*i+1] = (int16_t)(32768.0f * sample_r);
    }

    // buffer playback
    if (!perf_mode) audio_play_buffer(&a->info);
  }

  // cleanup
  if (!perf_mode) audio_cleanup(&a->info);
}


void audio_setup(audio** a) {
  *a = malloc(sizeof(audio));

  // clear out channel buffers
  for (int n = 0; n < num_tracks; n++) {
    for (int i = 0; i < track_size; i++) {
      (*a)->tracks[n][2*i]   = 0;
      (*a)->tracks[n][2*i+1] = 0;
    }
  }

  int rc;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params=NULL;
  snd_pcm_sw_params_t *sw=NULL;
  unsigned int sample_rate = 44100;
  int dir;
  snd_pcm_uframes_t frames = track_size/2-1;
  int16_t *buffer;
  int buffer_size;
    
  // Open PCM device for playback.
  rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
    exit(1);
  }

  // Allocate a hardware parameters object.
  snd_pcm_hw_params_malloc(&params);

  // Fill it in with default values.
  snd_pcm_hw_params_any(handle, params);

  // Set the desired hardware parameters.

  // Interleaved mode
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

  // Signed 16-bit little-endian format
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

  // Two tracks (stereo)
  snd_pcm_hw_params_set_channels(handle, params, 2);

  // 44100 bits/second sampling rate (CD quality)
  snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, &dir);

  // Set period size to 64 frames.
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

  // Write the parameters to the driver
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
    exit(1);
  }

  // allocate software params
  snd_pcm_sw_params_alloca(&sw);
  snd_pcm_sw_params_current(handle, sw);

  // start playing after the first write
  snd_pcm_sw_params_set_start_threshold(handle, sw, 1);

  // wake up on every interrupt
  snd_pcm_sw_params_set_avail_min(handle, sw, 1);

  // write config to driver
  snd_pcm_sw_params(handle, sw);

  // Use a buffer large enough to hold one period
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  buffer_size = frames*2; // 2 bytes/sample, 2 tracks
  buffer = (int16_t *)malloc(buffer_size*sizeof(int16_t));

  for (int i = 0; i < buffer_size; i++) {
    buffer[i] = 0;
  }

  (*a)->info.rc = rc;
  (*a)->info.handle = handle;
  (*a)->info.params = params;
  (*a)->info.sample_rate = sample_rate;
  (*a)->info.dir = dir;
  (*a)->info.frames = frames;
  (*a)->info.buffer = buffer;
  (*a)->info.buffer_size = buffer_size;
}

void audio_cleanup(audio_info* a) {
  snd_pcm_drain(a->handle);
  snd_pcm_close(a->handle);
}

void audio_play_buffer(audio_info* a) {
  a->rc = snd_pcm_writei(a->handle, a->buffer, a->frames);
  if (a->rc == -EPIPE) {
    // EPIPE means underrun
    fprintf(stderr, "underrun occurred\n");
    snd_pcm_prepare(a->handle);
  } else if (a->rc < 0) {
    fprintf(stderr, "error from writei: %s\n", snd_strerror(a->rc));
  } else if (a->rc != (int)a->frames) {
    fprintf(stderr, "short write, write %d frames\n", a->rc);
  }
}
