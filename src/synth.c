#include <stdio.h>
#include <stdlib.h>
#include "synth.h"


// Helper functions to deal with flags
void set_flag(note* note, note_flags flag) {
  note->flags |= (1 << flag);
}

void unset_flag(note* note, note_flags flag) {
  note->flags &= ~(1 << flag);
}

int check_flag(note* note, note_flags flag) {
  return note->flags & (1 << flag);
}


void synth_register_note(synth* s, float freq, float amp, note_event event, int key) {
  if (event == NOTE_ON) {
    fori(s->poly_count) {
      note* n = &s->notes[i];
      if (check_flag(n, NOTE_FREE)) {

	set_flag(n, NOTE_RESET);
	unset_flag(n, NOTE_FREE);
	unset_flag(n, NOTE_RELEASE);

	n->freq = freq;
	n->amp  = amp;
	n->key  = key;
	return;
      }
    }
  }

  if (event == NOTE_OFF) {
    fori(s->poly_count) {
      note* n = &s->notes[i];
      if (n->key == key) {
	set_flag(n, NOTE_RELEASE);
	return;
      }
    }
    puts("warning: got note off on non-existant key");
  }
}

float play_synth(cae_ctx* ctx, synth* s) {
  float sample = 0;
  fori(s->poly_count) {
    note* n = &s->notes[i];
    if (!check_flag(n, NOTE_FREE)) {
      sample += s->osc(ctx, s, i, &s->notes[i]);
    }
  }
  return sample;
}

synth* new_synth(int poly_count, osc_t osc){
  synth* s = malloc(sizeof(synth));
  s->poly_count = poly_count;
  s->notes = malloc(s->poly_count*sizeof(note));
  s->osc = osc;

  fori(s->poly_count) {
    note* n = &s->notes[i];

    set_flag(n, NOTE_FREE);
    unset_flag(n, NOTE_RESET);
    unset_flag(n, NOTE_RELEASE);
  }
  return s;
}