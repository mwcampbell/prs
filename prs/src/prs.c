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
 * prs.c: Implementation of an object which contains all other objects
 * necessary to run a radio station (mixer, scheduler, automation, and
 * database).
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "db.h"
#include "debug.h"
#include "mixer.h"
#include "mixerautomation.h"
#include "prs.h"
#include "scheduler.h"



typedef struct {
	int sock;
	PRS *prs;
} connection_data_blob;


PRS *
prs_new (void)
{
	PRS *prs = NULL;
	debug_printf (DEBUG_FLAGS_GENERAL, "prs_new called\n");
	prs = (PRS *) malloc (sizeof (PRS));
	assert (prs != NULL);
	prs->mixer = NULL;
	prs->automation = NULL;
	prs->scheduler = NULL;
	prs->telnet_interface = 0;
	prs->telnet_port = 0;
	prs->password = NULL;
	prs->mixer = mixer_new (512);
	prs->db = db_new ();
	mixer_sync_time (prs->mixer);
	prs->automation = mixer_automation_new (prs->mixer, prs->db);
	prs->scheduler = scheduler_new (prs->automation, prs->db,
					mixer_get_time (prs->mixer));
	prs->logger = NULL;
	return prs;
}



void
prs_start (PRS *prs)
{
	assert (prs != NULL);
	debug_printf (DEBUG_FLAGS_GENERAL, "prs_start_called\n");
	mixer_start (prs->mixer);
	scheduler_start (prs->scheduler, 300);
	mixer_automation_start (prs->automation);
}



void
prs_destroy (PRS *prs)
{
	assert (prs != NULL);
	debug_printf (DEBUG_FLAGS_GENERAL, "prs_destroy called\n");
	if (prs->scheduler != NULL)
		scheduler_destroy (prs->scheduler);
	if (prs->automation != NULL)
		mixer_automation_destroy (prs->automation);
	if (prs->db != NULL)
		db_close (prs->db);
	if (prs->mixer != NULL)
		mixer_destroy (prs->mixer);
	if (prs->password != NULL)
		free (prs->password);
	free (prs);
}
