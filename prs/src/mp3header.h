/* Based on mp3.h from libshout.  Modified for PRS by Matt Campbell. */

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
