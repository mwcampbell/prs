#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "ossmixeroutput.h"
#include "soundcard.h"



typedef struct {
	int fd;
} oss_info;



static void
oss_mixer_output_free_data (MixerOutput *o)
{
	oss_info *i;
	int tmp;

	assert (o != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "oss_mixer_output_free_data called for %s\n",
		      o->name);
	assert (o->data != NULL);
	i = (oss_info *) o->data;
	if (!soundcard_get_duplex ()) {
		close (i->fd);
		soundcard_set_fd (-1);
	}
	free (o->data);
}



static void
oss_mixer_output_post_data (MixerOutput *o)
{
	oss_info *i;

	assert (o != NULL);
	i = (oss_info *) o->data;

	if (write (i->fd, o->buffer, o->buffer_size * sizeof (short)*o->channels) < 0)
		debug_printf (DEBUG_FLAGS_MIXER,
			      "oss_mixer_output_post_data: write error: %s\n",
			      strerror (errno));
}



MixerOutput *
oss_mixer_output_new (const char *name,
		      int rate,
		      int channels,
		      int latency)
{
	MixerOutput *o;
	oss_info *i;
	int tmp;
	int fragment_size;

	assert (name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "oss_mixer_output_new (\"%s\", %d, %d, %d)\n",
		      name, rate, channels, latency);
	assert (rate > 0);
	assert ((channels == 1) || (channels == 2));
	assert (latency > 0);
	i = malloc (sizeof (oss_info));
	assert (i != NULL);

	/* Open the sound device */

	i->fd = soundcard_get_fd ();
	if (i->fd < 0) {
		i->fd = open ("/dev/dsp", O_RDWR);
		if (i->fd < 0) {
			soundcard_set_duplex (0);
			i->fd = open ("/dev/dsp", O_WRONLY);
		}
		else
			soundcard_set_duplex (1);
		if (i->fd < 0) {
			perror ("sound card open failed");
			exit (EXIT_FAILURE);
		}
		soundcard_set_fd (i->fd);
		soundcard_set_rate (rate);
		soundcard_set_channels (channels);
		
		/* Setup sound card */

		fragment_size = log (2*(latency/44100.0)*rate * sizeof (short) )  / log (2);
		tmp = 0x00010000|fragment_size;
		if (ioctl (i->fd, SNDCTL_DSP_SETFRAGMENT, &tmp) < 0) {
			perror ("ioctl on sound card failed");
			exit (EXIT_FAILURE);
		}
		tmp = AFMT_S16_LE;
		if (ioctl (i->fd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0) {
			perror ("ioctl on sound card failed");
			exit (EXIT_FAILURE);
		}
		if (channels == 1)
			tmp = 0;
		else
			tmp = 1;
		if (ioctl (i->fd, SNDCTL_DSP_STEREO, &tmp) < 0) {
			perror ("ioctl on sound card failed");
			exit (EXIT_FAILURE);
		}
		tmp = rate;
		if (ioctl (i->fd, SNDCTL_DSP_SPEED, &tmp) < 0) {
			perror ("ioctl on sound card failed");
			exit (EXIT_FAILURE);
		}
	}

	o = malloc (sizeof (MixerOutput));
	assert (o != NULL);
	o->name = strdup (name);
	o->rate = rate;
	o->channels = channels;
	o->data = (void *) i;
	o->enabled = 1;

	/* Overrideable methods */

	o->free_data = oss_mixer_output_free_data;
	o->post_data = oss_mixer_output_post_data;

	mixer_output_alloc_buffer (o, latency);
	return o;
}
