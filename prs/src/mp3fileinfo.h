#ifndef _MP3_FILE_INFO_H_
#define _MP3_FILE_INFO_H_
#include "fileinfo.h"




FileInfo *
mp3_file_info_new (char *path, unsigned short in_threshhold,
		   unsigned short out_threshhold);
  
#endif
