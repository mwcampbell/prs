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
 * scheduler.h: Header file for the scheduler object.
 *
 */

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

	/* Scheduled channel deletes */

	time_t scheduled_delete_time;
	int scheduled_delete_key;
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
indent-region
void
scheduler_reset (scheduler *s);



#endif
