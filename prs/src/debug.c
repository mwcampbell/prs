#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
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

	va_start (args, format);
	gettimeofday (&v, NULL);
	timestring = strdup (ctime (&v.tv_sec));
	timestring[strlen(timestring)-6] = 0;
	if (flags & debug_flags) {
		fprintf (stdout, "%s.%03d\n\t", timestring, v.tv_usec/1000);
		vfprintf (stdout, format, args);
	}
	free (timestring);
	va_end (args);
}
