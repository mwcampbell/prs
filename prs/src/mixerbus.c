#include <string.h>
#include "mixerbus.h"
#include "mixeroutput.h"



MixerBus *
mixer_bus_new (const char *name,
	       int rate,
	       int channels)
{
  MixerBus *b;

  b = (MixerBus *) malloc (sizeof(MixerBus));
  if (name)
    b->name = strdup (name);
  else
    b->name = NULL;
  b->rate = rate;
  b->channels = channels;
  b->buffer_size = b->process_buffer_size = b->rate*b->channels*MIXER_LATENCY;
  b->buffer = (short *) malloc (b->buffer_size*sizeof(short));
  b->process_buffer = (short *) malloc (b->buffer_size*sizeof(short));
  b->filters = b->outputs = NULL;
  b->enabled = 1;
  return b;
}



void
mixer_bus_destroy (MixerBus *b)
{
  if (!b)
    return;
  if (b->name)
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
  
  if (!b)
    return 0;

  if (!b->buffer)
    return 0;

  tmp = b->buffer;
  tmp2 = buffer;

  while (length--)
    {
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

  if (!b)
    return;
  for (tmp = b->filters; tmp; tmp = tmp->next)
    {
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
  for (tmp = b->outputs; tmp; tmp = tmp->next)
    {
      mixer_output_add_data ((MixerOutput *) (tmp->data),
			     b->buffer,
			     b->buffer_size);
    }
}



void
mixer_bus_reset_data (MixerBus *b)
{
  if (!b)
    return;
  if (!b->buffer)
    return;
  memset (b->buffer, 0, (b->buffer_size)*sizeof(short));
  memset (b->process_buffer, 0, (b->process_buffer_size)*sizeof(short));
}



void
mixer_bus_add_output (MixerBus *b,
		      MixerOutput *o)
{
  if (!b)
    return;
  b->outputs = list_append (b->outputs, o);
}



void
mixer_bus_add_filter (MixerBus *b,
			 AudioFilter *f)
{
  if (!b)
    return;
  b->filters = list_append (b->filters, f);
}
