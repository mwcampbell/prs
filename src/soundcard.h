/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

enum {
	soundcard_read = 1,
	soundcard_write
};


int
soundcard_setup (const char *name,
		 int rate,
		 int channels,
		 int mode,
		 int latency);
