#ifndef _MIXER_AUTOMATION_H
#define _MIXER_AUTOMATION_H
#include <pthread.h>
#include "list.h"
#include "mixer.h"
#include "vorbismixerchannel.h"



typedef enum {
  AUTOMATION_EVENT_TYPE_UNDEFINED,
  AUTOMATION_EVENT_TYPE_ADD_CHANNEL,
  AUTOMATION_EVENT_TYPE_FADE_CHANNEL,
  AUTOMATION_EVENT_TYPE_FADE_ALL,
  AUTOMATION_EVENT_TYPE_DELETE_ALL  
} AUTOMATION_EVENT_TYPE;



typedef struct {
  AUTOMATION_EVENT_TYPE type;
  double delta_time;
  double length;
  char *channel_name;
  double level;
  char *detail1;
  char *detail2;
} AutomationEvent;



AutomationEvent *
automation_event_new (void);
void
automation_event_destroy (AutomationEvent *e);



typedef struct {
  mixer *m;

  int running;
  pthread_t automation_thread;
  double last_event_time;
  list *events;
} MixerAutomation;



MixerAutomation *
mixer_automation_new (mixer *m);
void
mixer_automation_destroy (MixerAutomation *a);
int
mixer_automation_add_event (MixerAutomation *a,
			    AutomationEvent *e,
			    double start_time);
void
mixer_automation_next_event (MixerAutomation *a);
int
mixer_automation_start (MixerAutomation *a);
int
mixer_automation_stop (MixerAutomation *a);



#endif
