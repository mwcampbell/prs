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
 * wavefileinfo.c: Gets metadata from a wave file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileinfo.h"
#include "wavefileinfo.h"
#include "wave.h"


static double
get_wave_audio_in (FILE *fp,
		   format_header *format_chunk,
		   chunk_header *data_chunk_header,
		   int threshhold)
{
	char *buffer, *end_buffer;
	int buffer_size, bytes_read;
	short *ptr;
	double audio_in = 0.0;
	int done = 0;
  
	buffer_size = format_chunk->samples_per_second;
	buffer = (char *) malloc (buffer_size);

	fseek (fp, WAVE_HEADER_SIZE, SEEK_SET);
	while (!done) {
		bytes_read = fread (&buffer, buffer_size, 1, fp);
		end_buffer = buffer+bytes_read;
		ptr = (short *) buffer;
		while (ptr < (short *) end_buffer) {
			if (*ptr > threshhold || *ptr < -threshhold) {
				audio_in += (double) (ptr-(short *) buffer)/format_chunk->samples_per_second;
				done = 1;
				break;
			}
			ptr++;
		}
		if (!done)
			audio_in += (double) bytes_read/sizeof(short)/format_chunk->samples_per_second;
	}
	free (buffer);
	return audio_in;
}



static double
get_wave_audio_out (FILE *fp,
		    format_header *format_chunk,
		    chunk_header *data_chunk_header,
		    int threshhold)
{
	char *buffer, *end_buffer;
	int buffer_size, bytes_read;
	short *ptr;
	double audio_out;
	double seek_time;
  
	buffer_size = format_chunk->samples_per_second*10;
	buffer = (char *) malloc (buffer_size);

	seek_time = data_chunk_header->len/sizeof(short)/format_chunk->samples_per_second;
	
	seek_time -= 10;
	if (seek_time < 0)
		seek_time = 0;
	fseek (fp,
	       WAVE_HEADER_SIZE+(seek_time*sizeof(short)*format_chunk->samples_per_second),
	       SEEK_SET);
	
	bytes_read = fread (&buffer, buffer_size, 1, fp);
	end_buffer = buffer+bytes_read;
	ptr = (short *) end_buffer-1;
	while (ptr > (short *) buffer) {
		if (*ptr > threshhold || *ptr < -threshhold)
			break;
		ptr--;
	}
	audio_out = seek_time + (double) (ptr-(short *)buffer)/format_chunk->samples_per_second;
	free (buffer);
	return audio_out;
}



FileInfo *
wave_file_info_new (char *path,
		    unsigned short in_threshhold,
		    unsigned short out_threshhold)
{
	FILE *fp;
	FileInfo *info;
	riff_header riff;
	chunk_header format_chunk_header;
	format_header format_chunk;
	chunk_header data_chunk_header;
	
	fp = fopen (path, "rb");
	if (!fp)
		return NULL;

	/* Read the headers */

	fread (&riff, sizeof(riff_header), 1, fp);
	fread (&format_chunk_header, sizeof(chunk_header), 1, fp);
	fread (&format_chunk, sizeof(format_header), 1, fp);
	fread (&data_chunk_header, sizeof(chunk_header), 1, fp);

        /* Allocate the FileInfo structure */

	info = (FileInfo *) malloc(sizeof(FileInfo));
	if (!info) {
		return NULL;
	}
  
	/* Fill in structure */

	info->name = NULL;
	info->path = strdup (path);
	info->artist = NULL;
	info->genre = NULL;
	info->date = NULL;
	info->album = NULL;
	info->track_number = NULL;

	info->rate = format_chunk.samples_per_second/format_chunk.channels;
	info->channels = format_chunk.channels;
	if (in_threshhold > 0)
		info->audio_in = get_wave_audio_in (fp,
						    &format_chunk,
						    &data_chunk_header,
						    in_threshhold);
	else
		info->audio_in = -1.0;
	if (out_threshhold > 0)
		info->audio_out = get_wave_audio_out (fp,
						      &format_chunk,
						      &data_chunk_header,
						      out_threshhold);
	else
		info->audio_out = -1.0;
  
	return info;
}






