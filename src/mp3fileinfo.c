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
#include <taglib/tag_c.h>
#include "debug.h"
#include "fileinfo.h"
#include "mp3decoder.h"
#include "mp3fileinfo.h"
#include "mp3header.h"



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
	d = mp3_decoder_new (info->path, 0, info->channels);
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
	d = mp3_decoder_new (info->path, seek_time, info->channels);
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
	TagLib_File *file = NULL;
	TagLib_Tag *tag = NULL;
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
	file = taglib_file_new(path);

	if (file != NULL &&
	    (tag = taglib_file_tag(file)) != NULL)
	{
		info->name = taglib_tag_title(tag);
		if (info->name != NULL)
			info->name = strdup(info->name);
		info->artist = taglib_tag_artist(tag);
		if (info->artist != NULL)
			info->artist = strdup(info->artist);
		info->album = taglib_tag_album(tag);
		if (info->album != NULL)
			info->album = strdup(info->album);
		info->genre = taglib_tag_genre(tag);
		if (info->genre != NULL)
			info->genre = strdup(info->genre);
		taglib_tag_free_strings();
	}
	if (file != NULL)
		taglib_file_free(file);
  
	if (in_threshhold > 0)
		info->audio_in = get_mp3_audio_in (info, in_threshhold);
	if (out_threshhold > 0)
		info->audio_out = get_mp3_audio_out (info, out_threshhold);
  
	return info;
}


