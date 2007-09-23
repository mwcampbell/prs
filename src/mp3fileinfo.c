/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <id3.h>
#include "debug.h"
#include "fileinfo.h"
#include "mp3decoder.h"
#include "mp3fileinfo.h"
#include "mp3header.h"



/* Ugly hack to get around C API change in id3lib 3.8 */
#ifdef HAVE_ID3FIELD_GETASCIIITEM
#define ID3FIELD_GETASCII ID3Field_GetASCII
#else
#define ID3FIELD_GETASCII(a, b, c) ID3Field_GetASCII ((a), (b), (c), 1)
#endif



static int
mp3_scan (FileInfo *info)
{
	int frames = 0, samples = 0;
	FILE *fp = NULL;
	mp3_header_t mh;
	int sample_rate = -1, channels = -1;

	assert (info != NULL);
	fp = fopen (info->path, "rb");
	if (fp == NULL)
		return -1;

	while (mp3_header_read (fp, &mh))
	{
		frames++;
		if (sample_rate == -1)
			sample_rate = mh.samplerate;
		if (channels == -1)
			channels = (mh.mode == MPEG_MODE_MONO) ? 1 : 2;
		samples += mh.samples;
		fseek (fp, mh.framesize, SEEK_CUR);
	}

	fclose (fp);
	info->length = (double) samples / sample_rate;
	info->rate = sample_rate;
	info->channels = channels;
	return frames;
}

static double
get_mp3_audio_in (FileInfo *info, int threshhold)
{
	MP3Decoder *d = NULL;
	short *buffer = NULL, *end_buffer = NULL;
	int buffer_size, samples_read;
	short *ptr;
	double audio_in = 0.0;
	int done = 0;

	assert (info != NULL);
	d = mp3_decoder_new (info->path, 0);
	assert (d != NULL);
	buffer_size = info->rate * info->channels;
	buffer = (short *) malloc (buffer_size * sizeof (short));
	assert (buffer != NULL);

	while (!done)
	{
		samples_read = mp3_decoder_get_data (d, buffer, buffer_size);
		if (!samples_read)
			done = 1;
		end_buffer = buffer + samples_read;
		ptr = buffer;
		while (ptr < end_buffer)
		{
			if (*ptr > threshhold || *ptr < -threshhold)
			{
				audio_in += (double) (ptr - buffer) / buffer_size;
				done = 1;
				break;
			}
			ptr++;
		}

		if (!done)
			audio_in += (double) samples_read / buffer_size;
	}
	mp3_decoder_destroy (d);
	free (buffer);
	return audio_in;
}



static double
get_mp3_audio_out (FileInfo *info, int threshhold)
{
	MP3Decoder *d = NULL;
	short *buffer = NULL;
	int buffer_size, samples_read, i;
	double audio_out;
	double seek_time;

	assert (info != NULL);
	buffer_size = info->rate * info->channels * 20;
	buffer = (short *) malloc (buffer_size * sizeof (short));
	assert (buffer != NULL);

	seek_time = info->length - 10;
	if (seek_time < 0)
		seek_time = 0;
	d = mp3_decoder_new (info->path, seek_time);
	assert (d != NULL);

	samples_read = mp3_decoder_get_data (d, buffer, buffer_size);
	assert (samples_read > 0);
	for (i = samples_read - 1; i > 0; i--)
	{
		if (buffer[i] > threshhold || buffer[i] < -threshhold)
			break;
	}
	audio_out =
		info->length - (double) (samples_read - i) / (info->rate * info->channels);
	mp3_decoder_destroy (d);
	free (buffer);
	return audio_out;
}



FileInfo *
mp3_file_info_new (const char *path, const unsigned short in_threshhold,
		   const unsigned short out_threshhold)
{
	FileInfo *info;
	ID3Tag *tag = NULL;
	ID3Frame *frame = NULL;
	ID3Field *field = NULL;
	int frames;
	assert (path != NULL);
	debug_printf (DEBUG_FLAGS_FILE_INFO,
		      "mp3_file_info_new (\"%s\", %d, %d)\n",
		      path, in_threshhold, out_threshhold);

	/* Allocate the FileInfo structure */

	info = (FileInfo *) malloc(sizeof(FileInfo));
	assert (info != NULL);
  
	/* Fill in structure */

	info->name = NULL;
	info->path = strdup (path);
	info->artist = NULL;
	info->genre = NULL;
	info->date = NULL;
	info->album = NULL;
	info->track_number = NULL;
	frames = mp3_scan (info);
	if (frames < 0) {
		free (info);
		return NULL;
	}
	info->audio_in = -1.0;
	info->audio_out = -1.0;

	/* Get ID3 info. */
	tag = ID3Tag_New ();
	ID3Tag_Link (tag, path);

	if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_TITLE)) != NULL &&
	    (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
	{
		char title[256];
		ID3FIELD_GETASCII (field, title, 256);
		info->name = strdup (title);
	}
  
	if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_LEADARTIST)) != NULL &&
	    (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
	{
		char artist[256];
		ID3FIELD_GETASCII (field, artist, 256);
		info->artist = strdup (artist);
	}
  
	if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_ALBUM)) != NULL &&
	    (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
	{
		char album[256];
		ID3FIELD_GETASCII (field, album, 256);
		info->album = strdup (album);
	}
  
	if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_YEAR)) != NULL &&
	    (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
	{
		char date[256];
		ID3FIELD_GETASCII (field, date, 256);
		info->date = strdup (date);
	}
  
	if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_CONTENTTYPE)) != NULL &&
	    (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
	{
		char genre[256];
		ID3FIELD_GETASCII (field, genre, 256);
		info->genre = strdup (genre);
	}
  
	ID3Tag_Delete (tag);
  
	if (in_threshhold > 0)
		info->audio_in = get_mp3_audio_in (info, in_threshhold);
	if (out_threshhold > 0)
		info->audio_out = get_mp3_audio_out (info, out_threshhold);
  
	return info;
}