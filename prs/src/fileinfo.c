#include <stdlib.h>
#include "fileinfo.h"



void
file_info_free (FileInfo *info)
{
  if (!info)
    return;
  if (info->name)
    free (info->name);
  if (info->path)
    free (info->path);
  if (info->artist)
    free (info->artist);
  if (info->genre)
    free (info->genre);
  if (info->date)
    free (info->date);
  if (info->track_number)
    free (info->track_number);
  if (info->album)
    free (info->album);
  free (info);
}
