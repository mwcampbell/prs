#ifndef _WAVE_FILE_INFO_H_
#define _WAVE_FILE_INFO_H_
#include "fileinfo.h"




FileInfo *
wave_file_info_new (char *path,
		      unsigned short in_threshhold,
		      unsigned short out_threshhold);
  
#endif
