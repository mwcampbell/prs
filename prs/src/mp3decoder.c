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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "debug.h"
#include "mp3decoder.h"



MP3Decoder *
mp3_decoder_new (const char *filename, double start_time)
{
	MP3Decoder *d = NULL;
	int decoder_output[2] = {-1, -1};
	int rv = -1;
	char start_time_str[10] = "0";

	assert (filename != NULL);
	debug_printf (DEBUG_FLAGS_CODEC,
		      "mp3_decoder_new (\"%s\", %lf)\n",
		      filename, start_time);
	assert (start_time >= 0);
	sprintf (start_time_str, "%lf", start_time);

	/* Create pipe */

	if (pipe (decoder_output) < 0) {
		debug_printf (DEBUG_FLAGS_CODEC,
			      "mp3_decoder_new: pipe failed: %s\n",
			      strerror (errno));
		return NULL;
	}

	/* Fork the decoder process */

	signal (SIGCHLD, SIG_IGN);

	rv = fork ();
	if (!rv) {
		signal (SIGPIPE, SIG_DFL);
		close (0);
		close (1);
		close (2);
		open ("/dev/null", O_RDONLY);
		dup (decoder_output[1]);
		open ("/dev/null", O_WRONLY);
		close (decoder_output[0]);
		close (decoder_output[1]);
		execlp ("madplay", "madplay", "-q", "-o", "raw:-", "-s", start_time_str,
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
	free (d);
}
