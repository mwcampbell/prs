/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _MP3_FILE_INFO_H_
#define _MP3_FILE_INFO_H_
#include "fileinfo.h"




FileInfo *
mp3_file_info_new (const char *path, const unsigned short in_threshhold,
		   const unsigned short out_threshhold);
  
#endif
