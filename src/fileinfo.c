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
#include "debug.h"
#include "fileinfo.h"
#include "vorbisfileinfo.h"
#include "mp3fileinfo.h"
#include "wavefileinfo.h"


typedef struct {
	char ext[5];
	FileInfoConstructor constructor;
} extension_constructor_link;


#define SUPPORTED_EXTENSIONS 4

extension_constructor_link links[] = {
	{".mp2", mp3_file_info_new},
	{".mp3", mp3_file_info_new},
	{".ogg", vorbis_file_info_new},
	{".wav", wave_file_info_new}
};


FileInfo * 
file_info_new (const char *path,
	       const unsigned short in_threshhold,
	       const unsigned short out_threshhold)
{
	FileInfo *info = NULL;
	char *tmp = NULL;
	char *lower_path;
	int i;
	int l;

	assert (path != NULL);
	debug_printf (DEBUG_FLAGS_FILE_INFO, "file_info_new (%s, %hu, %hu)\n",
		      path, in_threshhold, out_threshhold);
	lower_path = strdup (path);
	tmp = lower_path;
	while (*tmp)
	  {
	    *tmp = tolower (*tmp);
	    tmp++;
	  }
	l = strlen (lower_path);
	for (i = 0; i < SUPPORTED_EXTENSIONS; i++) {
	  tmp = strstr (lower_path, links[i].ext);
		if (tmp - lower_path == l - strlen (links[i].ext)) {
			info = links[i].constructor (path, in_threshhold,
						     out_threshhold);
			free (lower_path);
			return info;
		}
	}
	debug_printf (DEBUG_FLAGS_FILE_INFO,
		      "file_info_new: no match found\n");
	free (lower_path);
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
