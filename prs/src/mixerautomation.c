#include <malloc.h>
#include "mixerautomation.h"



MixerAutomation *
mixer_automation_new (mixer *m)
{
  MixerAutomation *a = (MixerAutomation *) malloc (sizeof (MixerAutomation));
  a->m = m;
  a->events = NULL;
  a->last_event_time = -1.0;
  a->automation_thread = -1;
  return a;
}



void
mixer_automation_destroy (MixerAutomation *a)
{
  free (a);
}
