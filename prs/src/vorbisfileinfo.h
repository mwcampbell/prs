#ifndef _VORBIS_FILE_INFO_H_
#define _VORBIS_FILE_INFO_H_
#include "fileinfo.h"




FileInfo *
get_vorbis_file_info (char *path,
		      unsigned short in_threshhold,
		      unsigned short out_threshhold);
  
#endif
