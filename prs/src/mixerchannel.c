#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "mixerchannel.h"



void
mixer_channel_destroy (MixerChannel *ch)
{
  if (!ch)
    return;
  if (ch->name)
    free (ch->name);
  if (ch->location)
    free (ch->location);
  if (ch->free_data)
    ch->free_data (ch);
  else
    {
      if (ch->data)
	free (ch->data);
    }

  /* We don't own the outputs, so just free the list */

  list_free (ch->outputs);
  free (ch);
}



int
mixer_channel_get_data (MixerChannel *ch,
		      short *buffer,
		      int size)
{
  int bytes_read, i;
  short *ptr = buffer;
  short *end_buffer;

  if (!ch)
    return 0;
  if (!ch->get_data)
    return 0;

  bytes_read = ch->get_data (ch, buffer, size);

  /* Fading and level processing */

  end_buffer = buffer+bytes_read;
  while (ptr < end_buffer)
    {
      *ptr++ *= ch->level;
      if (ch->channels == 2)
	*ptr++ *= ch->level;
      if ((ch->fade < 0 && ch->level <= ch->fade_destination) ||
	  (ch->fade > 0 && ch->level >= ch->fade_destination))
	ch->fade = 0.0;
      if (ch->fade != 0.0)
	ch->level += ch->fade;
    }
  return bytes_read;
}
