#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H


#define MIXER_LATENCY .01



typedef struct _MixerOutput MixerOutput;



struct _MixerOutput {
  char *name;
  
  int enabled;

  /* Sample information */

  int rate;
  int channels;

  /* output buffer */

  short *buffer;
  int buffer_size;

  /* user data */

  void *data;

  /* Overrideable methods */

  void (*free_data) (MixerOutput *o);
  void (*post_output) (MixerOutput *o);
};



void
mixer_output_destroy (MixerOutput *o);
void
mixer_output_alloc_buffer (MixerOutput *o);
int
mixer_output_add_output (MixerOutput *o,
			 short *buffer,
			 int length);
void
mixer_output_post_output (MixerOutput *o);
void
mixer_output_reset_output (MixerOutput *o);
#endif
