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
 * soundcard.c: Miscelaneous soundcard functions.
 *
 */

#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include "soundcard.h"




static int soundcard_fd = -1;
static int soundcard_rate;
static int soundcard_channels = -1;
static int soundcard_duplex = -1;

int
soundcard_setup (int rate,
		 int channels,
		 int latency)
{
	int fragment_size;
	int tmp;

	soundcard_fd = open ("/dev/dsp", O_RDWR);
	if (soundcard_fd < 0) {
		soundcard_duplex = 0;
		soundcard_fd = open ("/dev/dsp", O_WRONLY);
	}
	else
		soundcard_duplex = 1;
	if (soundcard_fd < 0) {
		perror ("sound card open failed");
		exit (EXIT_FAILURE);
	}

	/* Setup sound card */

	fragment_size = log ((latency/44100.0)*rate*channels*sizeof (short))/log (2);
	tmp = 0x00040000|fragment_size;
	if (ioctl (soundcard_fd, SNDCTL_DSP_SETFRAGMENT, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	tmp = AFMT_S16_LE;
	if (ioctl (soundcard_fd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	if (channels == 1)
		tmp = 0;
	else
		tmp = 1;
	if (ioctl (soundcard_fd, SNDCTL_DSP_STEREO, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	tmp = rate;
	if (ioctl (soundcard_fd, SNDCTL_DSP_SPEED, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	soundcard_rate = rate;
	soundcard_channels = channels;
	return soundcard_fd;
}


int
soundcard_get_fd (void)
{
	return soundcard_fd;
}


void
soundcard_set_fd (int fd)
{
	soundcard_fd = fd;
}


int
soundcard_get_rate (void)
{
	return soundcard_rate;
}


void
soundcard_set_rate (int rate)
{
	soundcard_rate = rate;
}


int
soundcard_get_channels (void)
{
	return soundcard_channels;
}


void
soundcard_set_channels (int channels)
{
	soundcard_channels = channels;
}


int
soundcard_get_duplex (void)
{
	return soundcard_duplex;
}


void
soundcard_set_duplex (int duplex)
{
	soundcard_duplex = duplex;
}
