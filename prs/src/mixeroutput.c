#include "mixeroutput.h"



void
mixer_output_destroy (MixerOutput *o)
{
  if (!o)
    return;
  if (o->name)
    free (o->name);
  if (o->buffer)
    free (o->buffer);
  if (o->free_data)
    o->free_data (o);
  else
    {
      if (o->data)
	free (o->data);
    }
  free (o);
}



void
mixer_output_alloc_buffer (MixerOutput *o)
{
  if (!o)
    return;
  o->buffer_size = o->process_buffer_size = o->rate*o->channels*MIXER_LATENCY;
  o->buffer = (short *) malloc (o->buffer_size*sizeof(short));
  o->process_buffer = (short *) malloc (o->buffer_size*sizeof(short));
}



int
mixer_output_add_output (MixerOutput *o,
			 short *buffer,
			 int length)
{
  short *tmp, *tmp2;
  
  if (!o)
    return 0;
  if (!o->buffer)
    return 0;

  tmp = o->buffer;
  tmp2 = buffer;

  while (length--)
    {
      *tmp++ += *tmp2++;
    }
}



void
mixer_output_post_output (MixerOutput *o)
{
  AudioFilter *f;
  list *tmp;

  if (!o)
    return;
  if (!o->post_output)
    return;
  for (tmp = o->filters; tmp; tmp = tmp->next)
    {
      AudioFilter *f = (AudioFilter *) tmp->data;
      short *tmpbuf = o->buffer;

      audio_filter_process_data (f,
				 o->buffer,
				 o->buffer_size,
				 o->process_buffer,
				 o->process_buffer_size);
      o->buffer = o->process_buffer;
      o->process_buffer = tmpbuf;
    }
  o->post_output (o);
}



void
mixer_output_reset_output (MixerOutput *o)
{
  if (!o)
    return;
  if (!o->buffer)
    return;
  memset (o->buffer, 0, (o->buffer_size)*sizeof(short));
  memset (o->process_buffer, 0, (o->process_buffer_size)*sizeof(short));
}



void
mixer_output_add_filter (MixerOutput *o,
			 AudioFilter *f)
{
  if (!o)
    return;
  o->filters = list_append (o->filters, f);
}
