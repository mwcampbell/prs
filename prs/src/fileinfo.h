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
 * fileinfo.h: Header file for maniuplating metadata retrieved from files.
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

typedef FileInfo * (*FileInfoConstructor) (char *path,
					   unsigned short in_threshhold,
					   unsigned short out_threshhold);

FileInfo * 
file_info_new (const char *path,
	       unsigned short in_threshhold,
	       unsigned short out_threshhold);
void
file_info_free (FileInfo *info);
  
#endif
