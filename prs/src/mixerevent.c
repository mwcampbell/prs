#include "mixerevent.h"



void
mixer_event_free (MixerEvent *e)
{
  if (!e)
    return;
  if (e->channel_name)
    free (e->channel_name);
  if (e->detail1)
    free (e->detail1);
  if (e->detail2)
    free (e->detail2);
  free (e);
}

