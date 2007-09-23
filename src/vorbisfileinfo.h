/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _VORBIS_FILE_INFO_H_
#define _VORBIS_FILE_INFO_H_
#include "fileinfo.h"




FileInfo *
vorbis_file_info_new (const char *path,
		      const unsigned short in_threshhold,
		      const unsigned short out_threshhold);
  
#endif
