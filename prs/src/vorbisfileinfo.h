#ifndef _VORBIS_FILE_INFO_H_
#define _VORBIS_FILE_INFO_H_
#include "fileinfo.h"




FileInfo *
vorbis_file_info_new (char *path,
		      unsigned short in_threshhold,
		      unsigned short out_threshhold);
  
#endif
