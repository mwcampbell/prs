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
  int recording_table_created, user_table_created;
  
  /* Create list of audio files */

  system ("find /shares/audio -name *.ogg > /tmp/audio.list");
  fp = fopen ("/tmp/audio.list", "rb");
  
  /* Connect  to prs database */

  connect_to_database ("prs");


  /* Set up list of users who will receive access to tables */

  users = string_list_prepend (NULL, "apache");

  /* Create tables */

  if (!check_recording_tables ())
    {
      recording_table_created = 1;
      create_recording_tables (NULL, users);
    }
  else
    recording_table_created = 0;
  if (!check_user_table ())
    {
      user_table_created = 1;
      create_user_table (NULL, users);
    }
  else
    user_table_created = 0;
  if (!check_playlist_tables ())
    create_playlist_tables (NULL, users);

  /* Loop through all files found */

  while (!feof (fp))
    {
      Recording *r;

      fgets (path, 1024, fp);
      if (feof (fp))
	break;
      path[strlen(path)-1] = 0;
      if (!recording_table_created)
	{
	  i = get_vorbis_file_info (path, 0);
	  r = find_recording_by_path (path);
	  if (!r || i->length-r->length <= .001)
	    {
	      file_info_free (i);
	      recording_free (r);
	      continue;
	    }
	  file_info_free (i);
	  if (r)
	    {
	      delete_recording (r);
	      recording_free (r);
	    }
	  i = get_vorbis_file_info (path, 1000);
	  r = (Recording *) malloc (sizeof(Recording));
	}
      else
	{
	  i = get_vorbis_file_info (path, 1000);
	  r = (Recording *) malloc (sizeof(Recording));
	}
      if (i->name)
	r->name = strdup (i->name);
      else
	r->name = strdup ("");
      r->path = strdup (i->path);
      if (i->artist)
	r->artist = strdup (i->artist);
      else
	r->artist = strdup ("");
      if (i->genre)
	r->category = strdup (i->genre);
      else
	r->category = strdup ("");
      if (i->date)
	r->date = strdup (i->date);
      else
	r->date = strdup ("");
      r->rate = i->rate;
      r->channels = i->channels;
      r->length = i->length;
      r->audio_in = i->audio_in;
      r->audio_out = i->audio_out;
      printf ("Adding %s.\n", r->name);
      add_recording (r);
      recording_free (r);
      file_info_free (i);
    }
}
