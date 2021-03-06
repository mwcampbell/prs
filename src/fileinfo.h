/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

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

typedef FileInfo * (*FileInfoConstructor) (const char *path,
					   const unsigned short in_threshhold,
					   const unsigned short out_threshhold);

FileInfo * 
file_info_new (const char *path,
	       unsigned short in_threshhold,
	       unsigned short out_threshhold);
void
file_info_free (FileInfo *info);
  
#endif
