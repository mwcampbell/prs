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
 * debug.c: PRS debugging functions.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
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
	fflush (stdout);
}
