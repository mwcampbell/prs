#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>
#include "fileinfo.h"
#include "vorbisfileinfo.h"


static int
vorbis_get_data (OggVorbis_File *vf,
		 char *buffer,
		 int length)
{
  int bytes_left = length;
  char *ptr = buffer;
  int bytes_read;
  int section;
 
  while (bytes_left > 0)
    {
      bytes_read = ov_read (vf, ptr, bytes_left, 0, sizeof (short), 1, &section);
      if (bytes_read <= 0)
	break;
      bytes_left -= bytes_read;
      ptr += bytes_read;
    }
  if (bytes_left)
    return length-bytes_left;
  else
    return length;
}




static double
get_vorbis_audio_in (OggVorbis_File *vf,
		     int threshhold)
{
  vorbis_info *vi;
  char *buffer, *end_buffer;
  int buffer_size, bytes_read;
  short *ptr;
  double audio_in = 0.0;
  int done = 0;
  
  vi = ov_info (vf, -1);

  buffer_size = vi->rate*vi->channels*sizeof (short);
  buffer = (char *) malloc (buffer_size);

  while (!done)
    {
      bytes_read = vorbis_get_data (vf, buffer, buffer_size);
      end_buffer = buffer+bytes_read;
      ptr = (short *) buffer;
      while (ptr < (short *) end_buffer)
	{
	  if (*ptr > threshhold || *ptr < -threshhold)
	    {
	      audio_in += (double) (ptr-(short *) buffer)/vi->rate/vi->channels;
	      done = 1;
	      break;
	    }
	  ptr++;
	}
    if (!done)
      audio_in += (double) bytes_read/sizeof(short)/vi->rate/vi->channels;
    }
  free (buffer);
  return audio_in;
}



static double
get_vorbis_audio_out (OggVorbis_File *vf,
		     int threshhold)
{
  vorbis_info *vi;
  char *buffer, *end_buffer;
  int buffer_size, bytes_read;
  short *ptr;
  double audio_out;
  double seek_time;
  
  vi = ov_info (vf, -1);

  buffer_size = vi->rate*vi->channels*sizeof (short)*10;
  buffer = (char *) malloc (buffer_size);

  seek_time = ov_time_total (vf, -1);
  seek_time -= 10;
  if (seek_time < 0)
    seek_time = 0;
  ov_time_seek (vf, seek_time);

  bytes_read = vorbis_get_data (vf, buffer, buffer_size);
  end_buffer = buffer+bytes_read;
  ptr = (short *) end_buffer-1;
  while (ptr > (short *) buffer)
    {
      if (*ptr > threshhold || *ptr < -threshhold)
	break;
      ptr--;
      }
  audio_out = seek_time + (double) (ptr-(short *)buffer)/vi->rate/vi->channels;
  free (buffer);
  return audio_out;
}



static void
process_vorbis_comments (OggVorbis_File *vf,
			 FileInfo *info);



static void
process_vorbis_comments (OggVorbis_File *vf,
			 FileInfo *info)
{
  vorbis_comment *comment;
  char **comment_strings;
  char *value;
 
  comment = ov_comment (vf, -1);
  for (comment_strings = comment->user_comments; *comment_strings; comment_strings++)
   {
     value = strchr(*comment_strings, '=');
    if (!value)
      continue;
    value++;
    if (!strncmp (*comment_strings, "TITLE", 5))
      info->name = strdup (value);
    else if (!strncmp(*comment_strings, "GENRE", 5))
      info->genre = strdup (value);
    else if (!strncmp (*comment_strings, "ARTIST", 6))
      info->artist = strdup (value);
    else if (!strncmp(*comment_strings, "DATE", 4))
      info->date = strdup (value);
    else if (!strncmp (*comment_strings, "TRACKNUMBER", 11))
      info->track_number = strdup (value);
    else if (!strncmp (*comment_strings, "ALBUM", 5))
      info->album = strdup (value);
   }
}



FileInfo *
get_vorbis_file_info (char *path,
		      unsigned short in_threshhold,
		      unsigned short out_threshhold)
{
  FILE *fp;
  OggVorbis_File vf;
  vorbis_info *vi;
  FileInfo *info;
  
  fp = fopen (path, "rb");
  if (!fp)
    return NULL;
  ov_open(fp, &vf, NULL, 0);
  vi = ov_info(&vf, -1);

  /* Allocate the FileInfo structure */

  info = (FileInfo *) malloc(sizeof(FileInfo));
  if (!info)
    {
      ov_clear (&vf);
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
  info->rate = vi->rate;
  info->channels = vi->channels;
  info->length = ov_time_total(&vf, -1);
  process_vorbis_comments (&vf, info);
  
  if (in_threshhold > 0)
    info->audio_in = get_vorbis_audio_in (&vf, in_threshhold);
  else
    info->audio_in = -1.0;
  if (out_threshhold > 0)
    info->audio_out = get_vorbis_audio_out (&vf, out_threshhold);
  else
    info->audio_out = -1.0;
  
  ov_clear(&vf);
  return info;
}






