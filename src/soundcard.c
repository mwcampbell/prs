/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include "soundcard.h"




int
soundcard_setup (const char *name,
		 int rate,
		 int channels,
		 int mode,
		 int latency)
{
	int fd;
	int fragment_size;
	int tmp;

	if ((mode & soundcard_read) && (mode & soundcard_write))
		mode = O_RDWR;
	else if (mode & soundcard_write)
		mode = O_WRONLY;
	else
		mode = O_RDONLY;
	fd = open (name, mode);
	if (fd < 0) {
		perror ("sound card open failed");
		exit (EXIT_FAILURE);
	}

	/* Setup sound card */

	fcntl (fd, F_SETFL, O_NONBLOCK);

	fragment_size = log ((latency/44100.0)*rate*channels*sizeof (short))/log (2);
	tmp = 0x00040000|fragment_size;
	if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	tmp = AFMT_S16_LE;
	if (ioctl (fd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	if (channels == 1)
		tmp = 0;
	else
		tmp = 1;
	if (ioctl (fd, SNDCTL_DSP_STEREO, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	tmp = rate;
	if (ioctl (fd, SNDCTL_DSP_SPEED, &tmp) < 0) {
		perror ("ioctl on sound card failed");
		exit (EXIT_FAILURE);
	}
	return fd;
}


