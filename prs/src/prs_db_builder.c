#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "fileinfo.h"
#include "mp3fileinfo.h"
#include "vorbisfileinfo.h"
#include "db.h"





int
main (int argc, char *argv[])
{
  char path[1024], find_cmd[1024];
  FILE *fp;
  FileInfoConstructor get_file_info;
  FileInfo *i;
  list *users;
  int recording_table_created, user_table_created;

  if (argc < 3)
    {
      fprintf (stderr, "Usage:  %s PATH WWW-USER [GENRE]\n", argv[0]);
      return 1;
    }
  
  /* Create list of audio files */

  nice (20);
  sprintf (find_cmd, "find \"%s\" -type f", argv[1]);
  fp = popen (find_cmd, "r");
  
  /* Set up list of users who will receive access to tables */

  users = string_list_prepend (NULL, argv[2]);

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
  if (!check_config_status_tables ())
    create_config_status_tables (NULL, users);
  if (!check_log_table ())
    create_log_table (NULL, users);
  
  /* Loop through all files found */

  while (!feof (fp))
    {
      Recording *r = NULL;
      char *ext = NULL;

      fgets (path, 1024, fp);
      if (feof (fp))
	break;
      path[strlen(path)-1] = 0;
      ext = path + strlen (path) - 4;
      if (strcmp (ext, ".mp3") == 0)
	get_file_info = get_mp3_file_info;
      else if (strcmp (ext, ".ogg") == 0)
	get_file_info = get_vorbis_file_info;
      else
	continue;

      if (!recording_table_created)
	{
	  i = get_file_info (path, 0);
	  r = find_recording_by_path (path);
	  if (r && abs (i->length-r->length) <= .001 &&
	      !strcmp (r->category, argc > 3 ? argv[3] : i->genre) &&
	      !strcmp (r->name, i->name))
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
	  i = get_file_info (path, 1000);
	  r = (Recording *) malloc (sizeof(Recording));
	}
      else
	{
	  i = get_file_info (path, 1000);
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
      if (argc > 3)
	r->category = strdup (argv[3]);
      else if (i->genre)
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

  pclose (fp);
  return 0;
}
