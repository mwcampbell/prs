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
  o->buffer_size = o->rate*o->channels*MIXER_LATENCY;
  o->buffer = (short *) malloc (o->buffer_size*sizeof(short));
}



int
mixer_output_add_data (MixerOutput *o,
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
mixer_output_post_data (MixerOutput *o)
{
  if (!o)
    return;
  if (!o->post_data)
    return;
  o->post_data (o);
}



void
mixer_output_reset_data (MixerOutput *o)
{
  if (!o)
    return;
  if (!o->buffer)
    return;
  memset (o->buffer, 0, (o->buffer_size)*sizeof(short));
}



