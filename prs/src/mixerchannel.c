#include <stdio.h>
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
    ch->free_data (ch->data);
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
  if (!ch)
    return 0;
  if (!ch->get_data)
    return 0;
  return ch->get_data (ch, buffer, size);
}
