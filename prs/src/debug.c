#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
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
	time_t cur_time;
	char *timestring;

	va_start (args, format);
	cur_time = time (NULL);
	timestring = ctime (&cur_time);
	if (flags & debug_flags) {
		fprintf (stdout, "%s\t", timestring);
		vfprintf (stdout, format, args);
	}
	va_end (args);
}
