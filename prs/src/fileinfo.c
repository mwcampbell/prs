#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
file_info_new (char *path,
	       unsigned short in_threshhold,
	       unsigned short out_threshhold)
{
	FileInfo *info = NULL;
	char *tmp = NULL;
	int i;
	int l;
	
	l = strlen (path);
	for (i = 0; i < SUPPORTED_EXTENSIONS; i++) {
		tmp = strstr (path, links[i].ext);
		if (tmp-path == l-4)
			info = links[i].constructor (path, in_threshhold, out_threshhold);
	}
	return info;
}


void
file_info_free (FileInfo *info)
{
	if (!info)
		return;
	if (info->name)
		free (info->name);
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
