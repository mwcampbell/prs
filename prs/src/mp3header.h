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
 * mp3header.h: header file for the code that retrieves and  parses mp3
 * headers.
 *
 * Based on mp3.h from libshout.  Modified for PRS by Matt Campbell.
 *
 */

#ifndef __MP3HEADER_H
#define __MP3HEADER_H

#include <stdio.h>

/*
*	MP3 Frame handling curtesy of Scott Manley - may he always be Manley.
*/

/* mp3 stuff */
#define MPEG_MODE_MONO 3

typedef struct mp3_header_St {
	int syncword;
	int layer;
	int version;
	int error_protection;
	int bitrate_index;
	int samplerate_index;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int stereo;
	int bitrate;
	int samplerate;
	int samples;
	int framesize;
} mp3_header_t;

int mp3_header_parse(unsigned long head, mp3_header_t *mh);
int mp3_header_read(FILE *fp, mp3_header_t *mh);

#endif /* __MP3HEADER_H */
