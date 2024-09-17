
#include "context.h"

typedef enum note_flags {
  NOTE_FREE    = 0, // whether note slot is free for a new note
  NOTE_RELEASE = 1, // whether note should start releasing
  NOTE_RESET   = 2, // whether synth should reset internals for this note index
} note_flags;


typedef struct note {
  float freq;
  float amp;

  int   key;   // used to identify note on/off-events
  int   flags;
} note;

void set_flag(note* note, note_flags flag);
void unset_flag(note* note, note_flags flag);
int  check_flag(note* note, note_flags flag);

struct synth; // Forward declare 'synth' for osc_t def
typedef float osc_t (cae_ctx* ctx, struct synth* s, int note_index, note* note);

typedef struct synth {
  osc_t* osc;

  int   poly_count;
  note* notes;
} synth;

typedef enum note_event {
  NOTE_ON,
  NOTE_OFF,
} note_event;

synth* new_synth(int poly_count, osc_t* osc);
void   synth_register_note(synth* s, float freq, float amp, note_event event, int key);
float  play_synth(cae_ctx* ctx, synth* s);