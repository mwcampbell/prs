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
 * shoutmixeroutput.c: Mixer output which streams to a shoutcast/icecast
 * server.
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/soundcard.h>
#include <shout/shout.h>
#include "debug.h"
#include "shoutmixeroutput.h"
#include "list.h"


typedef struct {
	shout_t *shout;
	int retry_delay;
	pid_t encoder_pid;
	int encoder_output_fd;
	int encoder_input_fd;
	list *args_list;
	int stream_reset;
	int stereo;
	pthread_t shout_thread_id;
	int archive_file_fd;
} shout_info;



static void
start_encoder (MixerOutput *o)
{
	shout_info *i;
	int encoder_output[2];
	int encoder_input[2];
	
	if (!o)
		return;
	if (!o->data)
		return;
	i = (shout_info *) o->data;
  
	/* Create pipes */

	pipe (encoder_output);
	pipe (encoder_input);

	/* Fork the encoder process */

	i->encoder_pid = fork ();
	if (!i->encoder_pid) {
		char *prog_name;
		char **args_array;
		char sample_rate_arg[128];
		char channels_arg[128];
		char bitrate_arg[128];
      
		sprintf (bitrate_arg, "-b%d", shout_get_bitrate (i->shout));
		i->args_list = string_list_prepend (i->args_list, bitrate_arg);

		if (shout_get_format (i->shout) == SHOUT_FORMAT_MP3) {
			prog_name = "lame";
			sprintf (sample_rate_arg, "-s%lf", (double) o->rate/1000);
			i->args_list = string_list_prepend (i->args_list,
							    "-r");
			i->args_list = string_list_prepend (i->args_list,
							    sample_rate_arg);
			i->args_list = string_list_prepend (i->args_list,
							    "-x");
			if (!i->stereo) {
				i->args_list = string_list_prepend (i->args_list,
								    "-mm");
				if (o->channels == 2)
					i->args_list = string_list_prepend (i->args_list,
									    "-a");
			}
		}
		else {
			prog_name = "oggenc";
			sprintf (sample_rate_arg, "-R%d", o->rate);
			sprintf (channels_arg, "-C%d", o->channels);
			i->args_list = string_list_prepend (i->args_list,
							    sample_rate_arg);
			i->args_list = string_list_prepend (i->args_list,
					     channels_arg);
		}
		
		i->args_list = string_list_prepend (i->args_list,
						    "-");
		if (shout_get_format (i->shout) == SHOUT_FORMAT_MP3)
			i->args_list = string_list_prepend (i->args_list,
							    "-");
		i->args_list = list_reverse (i->args_list);
		i->args_list = string_list_prepend (i->args_list,
						    prog_name);
		args_array = string_list_to_array (i->args_list);
		close (0);
		dup (encoder_input[0]);
		close (encoder_input[1]);
      
		close (1);
		close (2);
		dup (encoder_output[1]);
		close (encoder_output[0]);

		execvp (prog_name, args_array);
	}
	else {
		close (encoder_input[0]);
		i->encoder_input_fd = encoder_input[1];
		close (encoder_output[1]);
		i->encoder_output_fd = encoder_output[0];
	}
}



static void
stop_encoder (MixerOutput *o)
{
	shout_info *i;

	if (!o)
		return;
	if (!o->data)
		return;

	i = (shout_info *) o->data;
	close (i->encoder_input_fd);
	kill (i->encoder_pid, SIGTERM);
	waitpid (i->encoder_pid, NULL, 0);
}



static void
shout_mixer_output_free_data (MixerOutput *o)
{
	shout_info *i;

	assert (o != NULL);
	assert (o->data != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "shout_mixer_output_free_data called\n");
	i = (shout_info *) o->data;

	stop_encoder (o);
	i->stream_reset = 1;
	pthread_join (i->shout_thread_id, NULL);
	if (i->shout)
		shout_free (i->shout);
	if (i->args_list)
		string_list_free (i->args_list);
	if (i->archive_file_fd > 0)
		close (i->archive_file_fd);
	free (o->data);
}



static void *
shout_thread (void *data)
{
	MixerOutput *o = NULL;
	shout_info *i = NULL;
	int retry_delay;
	int blocks_waited = 0;
	int connected = 0;
	char buffer[1024];
	char *tmp;
	int bytes_read, bytes_left;
	
	assert (data != NULL);
	o = (MixerOutput *) data;
	i = (shout_info *) o->data;

	/*
	 *
	 * Guess roughly how many blocks to wait before retrying the
	 * connection if it's disconnected
	 *
	 */

	retry_delay = (double)(shout_get_bitrate(i->shout)*1000/8/1024)*i->retry_delay;

	while (!i->stream_reset) {
		tmp = buffer;
		bytes_left = 1024;

		while (bytes_left) {
			bytes_read = read (i->encoder_output_fd, tmp, bytes_left);
			if (bytes_read <= 0)
				break;
			tmp += bytes_read;
			bytes_left -= bytes_read;
		}
		if (!connected && !(blocks_waited%retry_delay)) {
			int rv;

			debug_printf (DEBUG_FLAGS_GENERAL, "Attempting to connect to server.\n");
			rv = shout_open (i->shout);
			if (!rv) {
				connected = 1;
				debug_printf (DEBUG_FLAGS_GENERAL, "Connected to server.\n");
			}
			else {
				debug_printf (DEBUG_FLAGS_GENERAL,
					      "Server connection attempt failed: %s\n", shout_get_error (i->shout));
				blocks_waited = 0;
			}
		}
		else if (connected) {
			int rv = shout_send (i->shout, buffer, 1024-bytes_left);
			if (rv) {
				debug_printf (DEBUG_FLAGS_GENERAL, "Error sending data to server: %s\n", shout_get_error (i->shout));
				shout_close (i->shout);
				connected = 0;
				blocks_waited = 0;
			}
		}
		else
			blocks_waited++;
		if (i->archive_file_fd > 0)
			write (i->archive_file_fd, buffer, 1024-bytes_left);
		if (bytes_left > 0)
			i->stream_reset = 1;
	}
}



static void
shout_mixer_output_post_data (MixerOutput *o)
{
	shout_info *i;
  
	assert (o != NULL);
	i = (shout_info *) o->data;

	if (i->shout_thread_id == 0)
		pthread_create (&(i->shout_thread_id), NULL, shout_thread, o);
	if (write (i->encoder_input_fd, o->buffer,
		   o->buffer_size*sizeof(short)*o->channels) < 0)
		debug_printf (DEBUG_FLAGS_MIXER,
			      "shout_mixer_output_post_data: write: %s\n",
			      strerror (errno));
}



MixerOutput *
shout_mixer_output_new (const char *name,
			const int rate,
			const int channels,
			const int latency,
			shout_t *s,
			const int stereo,
			list *encoder_args,
			const char *archive_file_name,
			double retry_delay)
{
	MixerOutput *o;
	shout_info *i;
  
	if (!s)
		return NULL;

	i = malloc (sizeof (shout_info));
	if (!i)
		return NULL;

	i->shout = s;
	i->stereo = stereo;
	i->args_list = encoder_args;
	i->stream_reset = 0;

	/* Open the archive file if one is specified */

	if (archive_file_name) {
		i->archive_file_fd = open (archive_file_name, O_WRONLY | O_CREAT | O_TRUNC);
	}
	else
		i->archive_file_fd = -1;

	i->retry_delay = retry_delay;

	o = malloc (sizeof (MixerOutput));
	if (!o) {
		free (i);
		return NULL;
	}
  
	o->name = strdup (name);
	o->rate = rate;
	o->channels = channels;
	o->data = (void *) i;
	o->enabled = 1;
  
	/* Overrideable methods */

	o->free_data = shout_mixer_output_free_data;
	o->post_data = shout_mixer_output_post_data;

	mixer_output_alloc_buffer (o, latency);

	i->shout_thread_id = 0;
	start_encoder (o);
	return o;
}



