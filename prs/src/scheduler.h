#ifndef _SCHEDULER_H
#define _SCHEDULER_H
#include <pthread.h>
#include "mixerautomation.h"
#include "db.h"



typedef struct {
  pthread_mutex_t mut;
  MixerAutomation *a;
  Database *db;
  double prev_event_start_time;
  double prev_event_end_time;
  double last_event_end_time;
  list *template_stack;

  /* Scheduler thread */

  pthread_t scheduler_thread;
  double preschedule;
  double running;
} scheduler;



scheduler *
scheduler_new (MixerAutomation *a, Database *db, double cur_time);
void
scheduler_destroy (scheduler *s);
double
scheduler_schedule_next_event (scheduler *s);
void
scheduler_start (scheduler *s,
		 double preschedule);



#endif
