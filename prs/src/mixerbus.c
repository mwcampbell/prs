#include <assert.h>
#include <string.h>
#include "debug.h"
#include "mixerbus.h"
#include "mixeroutput.h"



MixerBus *
mixer_bus_new (const char *name,
	       int rate,
	       int channels,
	       int latency)
{
	MixerBus *b = NULL;

	assert (name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_bus_new (\"%s\", %d, %d, %d)\n",
		      name, rate, channels, latency);
	assert (rate > 0);
	assert ((channels == 1) || (channels == 2));
	assert (latency > 0);
	b = (MixerBus *) malloc (sizeof (MixerBus));
	assert (b != NULL);
	b->name = strdup (name);
	b->rate = rate;
	b->channels = channels;
	b->buffer_size = b->process_buffer_size =
		latency/44100.0*rate;
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_bus_new: buffer_size = %d\n", b->buffer_size);
	b->buffer = (short *) malloc (b->buffer_size*sizeof(short)*channels);
	assert (b->buffer != NULL);
	b->process_buffer = (short *) malloc (b->buffer_size*sizeof(short)*channels);
	assert (b->process_buffer != NULL);
	b->filters = b->outputs = NULL;
	b->enabled = 1;
	return b;
}



void
mixer_bus_destroy (MixerBus *b)
{
	assert (b != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "destroying bus %s\n", b->name);
	free (b->name);
	if (b->buffer)
		free (b->buffer);
	if (b->process_buffer)
		free (b->process_buffer);
	free (b);
}



int
mixer_bus_add_data (MixerBus *b,
		    short *buffer,
		    int length)
{
	short *tmp, *tmp2;
	long samp;
  
	assert (b != NULL);
	assert (buffer != NULL);
	assert (length >= 0);
	assert (b->buffer != NULL);
	tmp = b->buffer;
	tmp2 = buffer;

	length *= b->channels;
	while (length--) {
		samp = *tmp+*tmp2++;
		if (samp > 32767)
			*tmp++ = 32767;
		else if (samp < -32768)
			*tmp++ = -32768;
		else
			*tmp++ = (short) samp;    
	}
}



void
mixer_bus_post_data (MixerBus *b)
{
	AudioFilter *filter;
	list *tmp;

	assert (b != NULL);
	for (tmp = b->filters; tmp; tmp = tmp->next) {
		short *tmpbuf = b->buffer;

		filter = tmp->data;
		audio_filter_process_data (filter,
					   b->buffer,
					   b->buffer_size,
					   b->process_buffer,
					   b->process_buffer_size);
		b->buffer = b->process_buffer;
		b->process_buffer = tmpbuf;
	}
	for (tmp = b->outputs; tmp; tmp = tmp->next) {
		mixer_output_add_data ((MixerOutput *) (tmp->data),
				       b->buffer,
				       b->buffer_size);
	}
}



void
mixer_bus_reset_data (MixerBus *b)
{
	assert (b != NULL);
	assert (b->buffer != NULL);
	memset (b->buffer, 0, (b->buffer_size)*sizeof(short)*b->channels);
	memset (b->process_buffer, 0, (b->process_buffer_size)*sizeof(short)*b->channels);
}



void
mixer_bus_add_output (MixerBus *b,
		      MixerOutput *o)
{
	assert (b != NULL);
	assert (o != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "adding output %s to bus %s\n",
		      o->name, b->name);
	b->outputs = list_append (b->outputs, o);
}



void
mixer_bus_add_filter (MixerBus *b,
		      AudioFilter *f)
{
	assert (b != NULL);
	assert (f != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "adding filter to bus %s\n",
		      b->name);
	b->filters = list_append (b->filters, f);
}
