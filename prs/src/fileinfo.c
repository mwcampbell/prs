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
 * fileinfo.c: Implementation of functions for maniuplating metadata
 * retrieved from audio files.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "fileinfo.h"
#include "vorbisfileinfo.h"
#include "mp3fileinfo.h"


typedef struct {
	char ext[5];
	FileInfoConstructor constructor;
} extension_constructor_link;


#define SUPPORTED_EXTENSIONS 2

extension_constructor_link links[] = {
	{".mp3", mp3_file_info_new},
	{".ogg", vorbis_file_info_new}
};


FileInfo * 
file_info_new (const char *path,
	       unsigned short in_threshhold,
	       unsigned short out_threshhold)
{
	FileInfo *info = NULL;
	char *tmp = NULL;
	int i;
	int l;

	assert (path != NULL);
	debug_printf (DEBUG_FLAGS_FILE_INFO, "file_info_new (%s, %hu, %hu)\n",
		      path, in_threshhold, out_threshhold);
	l = strlen (path);
	for (i = 0; i < SUPPORTED_EXTENSIONS; i++) {
		tmp = strstr (path, links[i].ext);
		if (tmp - path == l - strlen (links[i].ext)) {
			info = links[i].constructor (path, in_threshhold,
						     out_threshhold);
			return info;
		}
	}
	debug_printf (DEBUG_FLAGS_FILE_INFO,
		      "file_info_new: no match found\n");
	return NULL;
}


void
file_info_free (FileInfo *info)
{
	assert (info != NULL);
	if (info->name) {
		debug_printf (DEBUG_FLAGS_GENERAL,
			      "file_info_free called on %s\n",
			      info->name);
		free (info->name);
	} else
		debug_printf (DEBUG_FLAGS_GENERAL,
			      "file_info_free called on unnamed recording\n");
	if (info->path)
		free (info->path);
	if (info->artist)
		free (info->artist);
	if (info->genre)
		free (info->genre);
	if (info->date)
		free (info->date);
	if (info->track_number)
		free (info->track_number);
	if (info->album)
		free (info->album);
	free (info);
}
