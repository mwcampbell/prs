/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * mixerautomation.h: header file for the mixer automation object.
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
	logger *l;
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



#endif
