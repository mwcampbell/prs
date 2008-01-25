/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "debug.h"



static int debug_flags = 0;



void
debug_set_flags (int flags)
{
	debug_flags = flags;
}



void
debug_printf (int flags,
	      const char *format,
	      ...)
{
	va_list args;
	struct timeval v;
	char *timestring;

	if (!(flags & debug_flags))
		return;
	gettimeofday (&v, NULL);
	timestring = strdup (ctime (&v.tv_sec));
	timestring[strlen(timestring)-6] = 0;
	fprintf (stdout, "%s.%03ld\n\t", timestring, v.tv_usec/1000);
	free (timestring);
	va_start (args, format);
	vfprintf (stdout, format, args);
	va_end (args);
	fflush (stdout);
}
