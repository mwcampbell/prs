#include "audiofilter.h"



AudioFilter *
audio_filter_new (int rate,
		  int channels,
		  int buffer_size)
{
  AudioFilter *f = malloc (sizeof (AudioFilter));

  if (!f)
    return NULL;
  f->rate = rate;
  f->channels = channels;
  f->buffer = (short *) malloc (buffer_size*sizeof(short));
  f->buffer_size = buffer_size;
  f->buffer_length = 0;
  f->data = NULL;
  f->process_data = NULL;
  return f;
}



int
audio_filter_process_data (AudioFilter *f,
			   short *input,
			   int input_length,
			   short *output,
			   int output_length)
{
  if (!f)
    return 0;
  if (!f->process_data)
    return 0;
  return f->process_data (f,
			   input,
			   input_length,
			   output,
			   output_length);
}
  
