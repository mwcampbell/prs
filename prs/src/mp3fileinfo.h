#ifndef _MP3_FILE_INFO_H_
#define _MP3_FILE_INFO_H_
#include "fileinfo.h"




FileInfo *
get_mp3_file_info (char *path, unsigned short in_threshhold,
		   unsigned short out_threshhold);
  
#endif
