#ifndef _MIXER_AUTOMATION_H
#define _MIXER_AUTOMATION_H
#include <pthread.h>
#include "list.h"
#include "mixer.h"



typedef struct {
  mixer *m;

  pthread_t automation_thread;
  double last_event_time;
  list *events;
} MixerAutomation;



MixerAutomation *
mixer_automation_new (mixer *m);
void
mixer_automation_destroy (MixerAutomation *a);



#endif
