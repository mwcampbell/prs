#ifndef _FILE_INFO_H_
#define _FILE_INFO_H_




typedef struct {
  char *name;
  char *path;
  char *artist;
  char *genre;
  char *date;
  char *track_number;
  char *album;
  int rate;
  int channels;
  double length;
  double audio_in;
  double audio_out;
} FileInfo;

void
file_info_free (FileInfo *info);
FileInfo *
get_vorbis_file_info (char *path,
		      unsigned short threshhold);
  
#endif
