#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/soundcard.h>
#include <shout/shout.h>
#include "shoutmixeroutput.h"
#include "list.h"


typedef struct {
	shout_t *shout;
	pid_t encoder_pid;
	int encoder_output_fd;
	int encoder_input_fd;
	list *args_list;
	int stream_reset;
	int stereo;
	pthread_t shout_thread_id;
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
	close (i->encoder_output_fd);
	waitpid (i->encoder_pid, NULL, 0);
}



static void
shout_mixer_output_free_data (MixerOutput *o)
{
	shout_info *i;

	if (!o)
		return;
	if (!o->data)
		return;
	i = (shout_info *) o->data;

	if (i->shout)
		shout_free (i->shout);
	stop_encoder (o);
	if (i->args_list)
		string_list_free (i->args_list);
	free (o->data);
}



static void *
shout_thread (void *data)
{
	MixerOutput *o = (MixerOutput *) data;
	shout_info *i = (shout_info *) o->data;
	char buffer[1024];
	int bytes_read;
  
	while (!i->stream_reset) {
		bytes_read = read (i->encoder_output_fd, buffer, 1024);
		if (bytes_read > 0) {
			shout_send (i->shout, buffer, bytes_read);
			shout_sync (i->shout);
		}
	}
	fprintf (stderr, "Shout thread exiting...\n");
}



static void
shout_mixer_output_post_data (MixerOutput *o)
{
	shout_info *i;
  
	if (!o)
		return;
	i = (shout_info *) o->data;

	if (i->shout_thread_id == 0)
		pthread_create (&i->shout_thread_id, NULL, shout_thread, o);
	write (i->encoder_input_fd, o->buffer, o->buffer_size*sizeof(short));
}



MixerOutput *
shout_mixer_output_new (const char *name,
			const int rate,
			const int channels,
			const int latency,
			shout_t *s,
			const int stereo,
			list *encoder_args)
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
	if (shout_open (i->shout))
	{
		fprintf (stderr, "Couldn't connect to icecast server.\n");
		mixer_output_destroy (o);
		return NULL;
	}
	start_encoder (o);
	return o;
}



