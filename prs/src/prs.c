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


static void *
speex_connection_handler (void *data)
{
	int sock;
	PRS *prs;
	char *prompt = "Speex interface coming soon.\n";

	prs = ((connection_data_blob *) data)->prs;
	sock = ((connection_data_blob *) data)->sock;
	free (data);
	write (sock, prompt, strlen (prompt));
	close (sock);
}


static void *
speex_connection_listener (void *data)
{
	struct sockaddr_in sa;
	int sa_length = sizeof (sa);
	int listen_sock = -1, new_sock = -1;
	int reuse_addr = 1;
	
	/* Setup socket to listen */

	memset (&sa, 0, sizeof (sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons (12000);
	listen_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listen_sock < 0) {
		debug_printf (DEBUG_FLAGS_GENERAL, "Unable to bind to socket for speex connections.\n");
		return 0;
	}

	if (setsockopt (listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
		      sizeof (reuse_addr)) < 0) {
		debug_printf (DEBUG_FLAGS_GENERAL, "Can't set up socket options.\n");
		return 0;
	}

	if (bind (listen_sock, (struct sockaddr *) &sa, sizeof (sa)) < 0) {
		debug_printf (DEBUG_FLAGS_GENERAL, "Error binding to socket.\n");
		return;
	}

      if (listen (listen_sock, 1) < 0) {
	      debug_printf (DEBUG_FLAGS_GENERAL, "error attempting to listen on socket.\n");
	      return;
      }

      while ((new_sock = accept (listen_sock, (struct sockaddr *) &sa,
				 &sa_length)) >= 0) {
	      pthread_t thread;
	      connection_data_blob *b = (connection_data_blob *) malloc (sizeof(connection_data_blob));
	      b->prs = (PRS *) data;
	      b->sock = new_sock;
	      pthread_create (&thread, NULL, speex_connection_handler, b);
      }
}

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
	return prs;
}



void
prs_start (PRS *prs)
{
	assert (prs != NULL);
	debug_printf (DEBUG_FLAGS_GENERAL, "prs_start_called\n");
	mixer_start (prs->mixer);
	scheduler_start (prs->scheduler, 30);
	mixer_automation_start (prs->automation);
	pthread_create (&(prs->speex_connection_thread), NULL, speex_connection_listener, prs);
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
