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
 * mp3decoder.c: Implements an object to wrap the use of mpg123.
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "debug.h"
#include "mp3decoder.h"



MP3Decoder *
mp3_decoder_new (const char *filename, int start_frame)
{
	MP3Decoder *d = NULL;
	int decoder_output[2] = {-1, -1};
	int rv = -1;
	char start_frame_str[10] = "0";

	assert (filename != NULL);
	debug_printf (DEBUG_FLAGS_CODEC,
		      "mp3_decoder_new (\"%s\", %d)\n",
		      filename, start_frame);
	assert (start_frame >= 0);
	sprintf (start_frame_str, "%d", start_frame);

	/* Create pipe */

	if (pipe (decoder_output) < 0) {
		debug_printf (DEBUG_FLAGS_CODEC,
			      "mp3_decoder_new: pipe failed: %s\n",
			      strerror (errno));
		return NULL;
	}

	/* Fork the decoder process */

	rv = fork ();
	if (!rv) {
		close (0);
		close (1);
		close (2);
		dup2 (decoder_output[1], 1);
		close (decoder_output[0]);
		execlp ("mpg123", "mpg123", "-q", "-s", "-k", start_frame_str,
			filename, NULL);
		debug_printf (DEBUG_FLAGS_CODEC,
			      "mp3_decoder_new: exec failed: %s\n",
			      strerror (errno));
		exit (1);
	} else if (rv > 0) {
		close (decoder_output[1]);
		d = malloc (sizeof (MP3Decoder));
		assert (d != NULL);
		d->fd = decoder_output[0];
		d->pid = rv;
	} else
		debug_printf (DEBUG_FLAGS_CODEC,
			      "mp3_decoder_new: fork failed: %s\n",
			      strerror (errno));

	return d;
}



int
mp3_decoder_get_data (MP3Decoder *d, short *buffer, int length)
{
	int bytes_left = length * sizeof (short);
	char *ptr = (char *) buffer;
	int bytes_read;

	assert (d != NULL);
	assert (buffer != NULL);
	assert (length >= 0);

	while (bytes_left > 0) {
		bytes_read = read (d->fd, ptr, bytes_left);
		if (bytes_read < 0)
			debug_printf (DEBUG_FLAGS_CODEC,
				      "mp3_decoder_get_data: read error: %s\n",
				      strerror (errno));
		if (bytes_read <= 0)
			break;
		bytes_left -= bytes_read;
		ptr += bytes_read;
	}
	if (bytes_left)
		return length - bytes_left / sizeof (short);
	else
		return length;
}



void
mp3_decoder_destroy (MP3Decoder *d)
{
	assert (d != NULL);
	debug_printf (DEBUG_FLAGS_CODEC,
		      "mp3_decoder_destroy called\n");
	close (d->fd);
	kill (d->pid, SIGTERM);
	waitpid (d->pid, NULL, 0);
	free (d);
}
