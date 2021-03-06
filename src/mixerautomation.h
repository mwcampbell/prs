/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _MIXER_AUTOMATION_H
#define _MIXER_AUTOMATION_H
#include <pthread.h>
#include "list.h"
#include "mixer.h"
#include "mp3mixerchannel.h"
#include "vorbismixerchannel.h"
#include "db.h"
#include "logger.h"



typedef enum {
	AUTOMATION_EVENT_TYPE_UNDEFINED,
	AUTOMATION_EVENT_TYPE_ENABLE_CHANNEL,
	AUTOMATION_EVENT_TYPE_FADE_CHANNEL,
	AUTOMATION_EVENT_TYPE_FADE_ALL,
	AUTOMATION_EVENT_TYPE_DELETE_CHANNELS
} AUTOMATION_EVENT_TYPE;



typedef struct {
	AUTOMATION_EVENT_TYPE type;
	double delta_time;
	double length;
	char *channel_name;
	double level;
	int data;
	char *detail1;
	char *detail2;
} AutomationEvent;



AutomationEvent *
automation_event_new (void);
void
automation_event_destroy (AutomationEvent *e);



typedef struct {
	mixer *m;
	Database *db;

	pthread_mutex_t mut;
	int running;
	pthread_t automation_thread;
	double last_event_time;
	list *events;
	int logger_enabled;
	list *loggers;
} MixerAutomation;



MixerAutomation *
mixer_automation_new (mixer *m, Database *db);
void
mixer_automation_destroy (MixerAutomation *a);
int
mixer_automation_add_event (MixerAutomation *a,
			    AutomationEvent *e);
void
mixer_automation_next_event (MixerAutomation *a);
int
mixer_automation_start (MixerAutomation *a);
int
mixer_automation_stop (MixerAutomation *a);
void
mixer_automation_set_start_time (MixerAutomation *a,
				 double start_time);
double
mixer_automation_get_last_event_end (MixerAutomation *a);
void
mixer_automation_add_logger (MixerAutomation *a,
			     logger *l);
void
mixer_automation_enable_logger (MixerAutomation *a,
				int enabled);




#endif
