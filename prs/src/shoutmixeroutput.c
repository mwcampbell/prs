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



typedef struct {
	shout_t *shout;
	pid_t encoder_pid;
	int encoder_output_fd;
	int encoder_input_fd;
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
	if (!i->encoder_pid)
	{
		char sample_rate_arg[128];
		char channels_arg[128];
		char bitrate_arg[128];
      
		if (shout_get_format (i->shout) == SHOUT_FORMAT_MP3)
			sprintf (sample_rate_arg, "-s%lf", (double) o->rate/1000);
		else {
			sprintf (sample_rate_arg, "-R%d", o->rate);
			sprintf (channels_arg, "-C%d", o->channels);
		}
		
		sprintf (bitrate_arg, "-b%d", shout_get_bitrate (i->shout));
		close (0);
		dup (encoder_input[0]);
		close (encoder_input[1]);
      
		close (1);
		close (2);
		dup (encoder_output[1]);
		close (encoder_output[0]);

		if (shout_get_format (i->shout) == SHOUT_FORMAT_MP3)

                        /* Separate calls for stereo/mono encoding */

			if (i->stereo)
				execlp ("lame",
					"lame",
					"-r",
					sample_rate_arg,
					bitrate_arg,
					"-x",
					"-",
					"-",
					NULL);
			else
				execlp ("lame",
					"lame",
					"-r",
					sample_rate_arg,
					bitrate_arg,
					"-x",
					"-a",
					"-mm",
					"-",
					"-",
					NULL);
		else
			execlp ("oggenc",
				"oggenc",
				"-r",
				sample_rate_arg,
				channels_arg,
				bitrate_arg,
				"-",
				NULL);
	}
	else
	{
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
	free (o->data);
}



static void
shout_mixer_output_post_data (MixerOutput *o)
{
	shout_info *i;
  
	if (!o)
		return;
	i = (shout_info *) o->data;

	write (i->encoder_input_fd, o->buffer, o->buffer_size*sizeof(short));
}



static void *
shout_thread (void *data)
{
	MixerOutput *o = (MixerOutput *) data;
	shout_info *i = (shout_info *) o->data;
	char buffer[1024];
	int bytes_read;
  
	if (shout_open (i->shout))
	{
		fprintf (stderr, "Couldn'[t connect to icecast server.\n");
		o->enabled = 0;
		return;
	}
	fprintf (stderr, "Connected OK.\n");
	while (!i->stream_reset)
	{
		bytes_read = read (i->encoder_output_fd, buffer, 1024);
		if (bytes_read > 0)
		{
			shout_send (i->shout, buffer, bytes_read);
			shout_sync (i->shout);
		}
	}
	fprintf (stderr, "Shout thread exiting...\n");
}



MixerOutput *
shout_mixer_output_new (const char *name,
			int rate,
			int channels,
			shout_t *s,
			int stereo)
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

	mixer_output_alloc_buffer (o);

	pthread_create (&i->shout_thread_id, NULL, shout_thread, (void *) o);
	start_encoder (o);
	return o;
}



