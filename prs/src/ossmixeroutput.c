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
	if (i->fd < 0)
		i->fd = soundcard_setup (rate, channels, latency);
	if (i->fd < 0) {
		free (i);
		return NULL;
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
