
/*
  Cadence Audio Engine
  May 2024
  Børge Lundsaunet
*/

#include "context.h"
#include "platform_audio.h"
#include "gen.h"
#include "synth.h"


// Example synth
float test_osc(cae_ctx* ctx, synth* s, float freq, float amp, int index, int* reset);

int main(int argc, char** argv) {
  cae_ctx* ctx = malloc(sizeof(cae_ctx));

  platform_audio_setup(ctx);

  gen_table_init(ctx);

  // set up some toys to play with
  delay* d    = new_delay(ctx);
  synth* s    = new_synth(8, test_osc);
  sampler* sr = new_sampler();


  // Notes to play :)
  int n1;
  int n2;
  int n3;

  for (int j = 0; j < 1024*2; j++) {
    if (j == 0)     synth_register_note(s, 440.0f, 0.1, NOTE_ON, &n1);
    if (j == 100/2) synth_register_note(s, 660.0f, 0.1, NOTE_ON, &n2);
    if (j == 200/2) synth_register_note(s, 880.0f, 0.1, NOTE_ON, &n3);
    if (j == 600/2) synth_register_note(s, 440.0f, 0.1, NOTE_OFF, &n1);
    if (j == 700/2) synth_register_note(s, 660.0f, 0.1, NOTE_OFF, &n2);
    if (j == 800/2) synth_register_note(s, 880.0f, 0.1, NOTE_OFF, &n3);

    // -- 1. main processing loop, process the frames for the buffer
    fori(ctx->a->info.frames) {

      // first handle global generators
      process_gen_table(ctx);

      { // example synth :)
	float sample  = synth_play(ctx, s);
	float delayed = apply_delay(ctx, d, sample, 0.3, 0.6);
	write_to_track(ctx, 0, i, sample + delayed);
      }

      { // example sample
	float sample = play_sampler(sr);
	write_to_track(ctx, 2, i, 0.5*sample);
      }
    }

    // -- 2. mix tracks for playback
    mix_tracks(ctx);

    // -- 3. buffer playback
    platform_audio_play_buffer(ctx);
  }

  // cleanup
  platform_audio_cleanup(ctx);
}


// Test osc to demonstrate polyphony
float test_osc(cae_ctx* ctx, synth* s, float freq, float amp, int index, int* reset) {
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
    sine_i = register_gen_table(ctx, GEN_SINE);
    ctx->gt[sine_i].p->freq = 0.8;
  }

  if (*reset) {
    sines[index]->t = 0;
    phasors[index]->value = 0;
    *reset = 0;
  }

  phasors[index]->freq = 1.0f;
  float phase = gen_phasor(ctx, phasors[index]);
  amp *= 1.0f-phase;

  float mod = ctx->gt[sine_i].val;

  sines[index]->freq = freq;// + 15.0*mod;
  float sample = amp * gen_sine(ctx, sines[index]);
  return sample;
}
