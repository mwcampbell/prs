/* Based on mp3.c from libshout.  Modified for PRS by Matt Campbell. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef __MINGW32__
#include <sys/mman.h>
#endif
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include "mp3header.h"
  
/*
*	MP3 Frame handling curtesy of Scott Manley - may he always be Manley.
*/

/* begin const */
static const unsigned int bitrate[3][3][16] =
{
	{
		{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
		{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 }
	}, {
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }
	}, {
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }
	}
};

static const unsigned int samplerate[3][4] =
{
	{ 44100, 48000, 32000, 0 },
	{ 22050, 24000, 16000, 0 },
	{ 11025, 8000, 8000, 0 }
};

/* end const */

static void _parse_header(mp3_header_t *mh, unsigned long header)
{
	mh->syncword = (header >> 20) & 0x0fff;
	mh->version = ((header >> 19) & 0x01) ? 0 : 1;
	if ((mh->syncword & 0x01) == 0)
		mh->version = 2;
	mh->layer = 3 - ((header >> 17) & 0x03);
	mh->error_protection = ((header >> 16) & 0x01) ? 0 : 1;
	mh->bitrate_index = (header >> 12) & 0x0F;
	mh->samplerate_index = (header >> 10) & 0x03;
	mh->padding = (header >> 9) & 0x01;
	mh->extension = (header >> 8) & 0x01;
	mh->mode = (header >> 6) & 0x03;
	mh->mode_ext = (header >> 4) & 0x03;
	mh->copyright = (header >> 3) & 0x01;
	mh->original = (header >> 2) & 0x01;
	mh->emphasis = header & 0x03;
	
	mh->stereo = (mh->mode == MPEG_MODE_MONO) ? 1 : 2;
	mh->bitrate = bitrate[mh->version][mh->layer][mh->bitrate_index];
	mh->samplerate = samplerate[mh->version][mh->samplerate_index];
	
	if (mh->version == 0)
		mh->samples = 1152;
	else
		mh->samples = 576;

	if(mh->samplerate)
		mh->framesize = ((float)mh->samples * mh->bitrate * 1000 / (float)mh->samplerate) / 8 + mh->padding;
}

/* mp3 frame parsing stuff */
int mp3_header_parse(unsigned long head, mp3_header_t *mh)
{
	/* fill out the header struct */
	_parse_header(mh, head);

	/* check for syncword */
	if ((mh->syncword & 0x0ffe) != 0x0ffe)
		return 0;
	
	/* check for the right layer */
	if (mh->layer != 2)
		return 0;

	/* make sure bitrate is sane */
	if (mh->bitrate == 0)
		return 0;

	/* make sure samplerate is sane */
	if (mh->samplerate == 0)
		return 0;

	return 1;
}

int mp3_header_read(FILE *fp, mp3_header_t *mh)
{
	while (1) {
		unsigned char buf[4];
		unsigned long head;

		if (fread(buf, 4, 1, fp) <= 0)
			break;

		head = (buf[0] << 24) |
		       (buf[1] << 16) |
		       (buf[2] << 8) |
		       (buf[3]);
		memset(mh, 0, sizeof(mp3_header_t));

		if (mp3_header_parse(head, mh)) {
			fseek(fp, -4, SEEK_CUR);
			return 1;
		} else
			fseek(fp, -3, SEEK_CUR);
	}

	return 0;
}
