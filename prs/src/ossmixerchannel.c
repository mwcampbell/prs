#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include "mixerchannel.h"
#include "soundcard.h"



typedef struct {
	int fd;
} oss_info;



static int
oss_mixer_channel_get_data (MixerChannel *ch)
{
	oss_info *i;
	int remainder;
	short *tmp;
	int rv;
  
	if (!ch)
		return;
	tmp = ch->input;
	remainder = ch->chunk_size*ch->channels;
	i = (oss_info *) ch->data;
	while (remainder > 0) {
		rv = read (i->fd, tmp, remainder*sizeof(short));
		if (rv <= 0)
			break;
		remainder -= rv/sizeof(short);
		tmp += rv/sizeof(short);
	}
	return ch->chunk_size-(remainder/ch->channels);
}



static void
oss_mixer_channel_free_data (MixerChannel *ch)
{
	oss_info *i;
	int tmp;
  
	if (!ch)
		return;
	i = (oss_info *) ch->data;
	if (!i)
		return;
	if (!soundcard_get_duplex ()) {
		close (i->fd);
		soundcard_set_fd (-1);
	}
	free (i);
}



MixerChannel *
oss_mixer_channel_new (const char *name,
		       int rate,
		       int channels,
		       int mixer_latency)
{
	MixerChannel *ch;
	oss_info *i;
	int tmp;
	int fragment_size;
	
	i = malloc (sizeof (oss_info));
	if (!i)
		return NULL;

	/* Open the sound device */

	i->fd = soundcard_get_fd ();
	if (i->fd < 0) {
		i->fd = open ("/dev/dsp", O_RDWR);
		if (i->fd < 0) {
			soundcard_set_duplex (0);
			i->fd = open ("/dev/dsp", O_RDONLY);
		}
		else
			soundcard_set_duplex (1);
		soundcard_set_fd (i->fd);
		soundcard_set_rate (rate);
		soundcard_set_channels (channels);
		
		/* Setup sound card */

		fragment_size = log(2*(mixer_latency/44100.0)*rate*channels*sizeof(short))/log(2);
		tmp = 0x00010000|fragment_size;
		if (ioctl (i->fd, SNDCTL_DSP_SETFRAGMENT, &tmp) < 0) {
			close (i->fd);
			free (i);
			return NULL;
		}
		tmp = AFMT_S16_LE;
		if (ioctl (i->fd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0) {
			close (i->fd);
			free (i);
			return NULL;
		}
		if (channels == 1)
			tmp = 0;
		else
			tmp = 1;
		if (ioctl (i->fd, SNDCTL_DSP_STEREO, &tmp) < 0) {
			close (i->fd);
			free (i);
			return NULL;
		}
		tmp = rate;
		if (ioctl (i->fd, SNDCTL_DSP_SPEED, &tmp) < 0) {
			close (i->fd);
			free (i);
			return NULL;
		}
  
	}
	else {
		
		/* Make channel parameters match open sound card parameters */

		rate = soundcard_get_rate ();
		channels = soundcard_get_channels ();
	}
	if (i->fd < 0) {
		free (i);
		return NULL;
	}
	ch = mixer_channel_new (rate, channels, mixer_latency);
	if (!ch) {
		close (i->fd);
		free (i);
		return NULL;
	}
  
	ch->data = (void *) i;
	ch->name = strdup (name);
	ch->location = NULL;
  
	/* Overrideable methods */

	ch->get_data = oss_mixer_channel_get_data;
	ch->free_data = oss_mixer_channel_free_data;

	return ch;
}
