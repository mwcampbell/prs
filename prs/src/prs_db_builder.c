#include <stdio.h>
#include <string.h>
#include "fileinfo.h"
#include "db.h"





int main (void)
{
  char path[1024];
  FILE *fp;
  FileInfo *i;
  list *users;
  
  /* Create list of audio files */

  system ("find /shares/audio -name *.ogg > /tmp/audio.list");
  fp = fopen ("/tmp/audio.list", "rb");
  
  /* Connect  to prs database */

  connect_to_database ("prs");


  /* Set up list of users who will receive access to tables */

  users = string_list_prepend (NULL, "apache");

  /* Create tables */

  create_recording_table (NULL, users);
  create_user_table (NULL, users);
  create_playlist_template_table (NULL, users);
  create_playlist_event_table (NULL, users);

  /* Loop through all files found */

  while (!feof (fp))
    {
      RecordingInfo *ri = (RecordingInfo *) malloc (sizeof(RecordingInfo));
      fgets (path, 1024, fp);
      if (feof (fp))
	break;
      path[strlen(path)-1] = 0;
      i = get_vorbis_file_info (path, 1000);
      if (i->name)
	ri->name = strdup (i->name);
      else
	ri->name = strdup ("");
      ri->path = strdup (i->path);
      if (i->artist)
	ri->artist = strdup (i->artist);
      else
	ri->artist = strdup ("");
      if (i->genre)
	ri->category = strdup (i->genre);
      else
	ri->category = strdup ("");
      if (i->date)
	ri->date = strdup (i->date);
      else
	ri->date = strdup ("");
      ri->rate = i->rate;
      ri->channels = i->channels;
      ri->length = i->length;
      ri->audio_in = i->audio_in;
      ri->audio_out = i->audio_out;
      add_recording (ri);
      recording_info_free (ri);
      file_info_free (i);
    }
}
