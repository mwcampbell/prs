#ifndef _AUDIO_FILTER_H
#define _AUDIO_FILTER_H

#include <stdio.h>



typedef struct _AudioFilter AudioFilter;

struct _AudioFilter {
  int rate;
  int channels;
  short *buffer;
  int buffer_size;
  int buffer_length;
  void *data;

  /* Functions */

  int (*process_data) (AudioFilter *filter,
		       short *input_buffer,
		       int input_size,
		       short *output_buffer,
		       int output_size);
};




AudioFilter *
audio_filter_new (int rate,
		  int channels,
		  int buffer_size);
int
audio_filter_process_data (AudioFilter *f,
			   short *input,
			   int input_length,
			   short *output,
			   int output_length);



#endif
