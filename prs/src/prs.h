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
 * prs.h: header file for an object which contains all other objects
 * necessary to run a radio station (mixer, scheduler, automation, and
 * database).
 *
 */

#ifndef _PRS_H
#define _PRS_H
#include <pthread.h>
#include "mixer.h"
#include "db.h"
#include "mixerautomation.h"
#include "scheduler.h"
#include "logger.h"



typedef struct
{
	mixer *mixer;
	Database *db;
	MixerAutomation *automation;
	scheduler *scheduler;
	int telnet_interface;
	int telnet_port;
	char *password;;
	logger *logger;
	pthread_t speex_connection_thread;
	
}
PRS;



PRS *
prs_new (void);
void
prs_start (PRS *prs);
void
prs_destroy (PRS *prs);

#endif
