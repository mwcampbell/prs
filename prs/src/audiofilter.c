#include <assert.h>
#include <stdlib.h>
#include "debug.h"
#include "audiofilter.h"



AudioFilter *
audio_filter_new (int rate,
		  int channels,
		  int latency)
{
	AudioFilter *f = NULL;

	debug_printf (DEBUG_FLAGS_MIXER,
		      "audio_filter_new (%d, %d, %d)\n",
		      rate, channels, latency);
	assert (rate > 0);
	assert (channels > 0);
	assert (latency > 0);
	f = (AudioFilter *) malloc (sizeof (AudioFilter));
	assert (f != NULL);
	f->rate = rate;
	f->channels = channels;
	f->buffer_size = latency/(88200/(rate*channels));
	debug_printf (DEBUG_FLAGS_MIXER,
		      "audio_filter_new: buffer_size = %d\n",
		      f->buffer_size);
	f->buffer = (short *) malloc (f->buffer_size*sizeof(short));
	assert (f->buffer != NULL);
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
	assert (f != NULL);
	assert (f->process_data != NULL);
	assert (input != NULL);
	assert (input_length >= 0);
	assert (output != NULL);
	assert (output_length >= 0);
	return f->process_data (f,
				input,
				input_length,
				output,
				output_length);
}
