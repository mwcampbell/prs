#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "fileinfo.h"
#include "db.h"





static int
config (Database *db, const char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr cur;

  doc = xmlParseFile (filename);
  if (doc == NULL)
    {
      fprintf (stderr, "Can't process configuration file.\n");
      return -1;
    }
  cur = xmlDocGetRootElement (doc);
  if (cur == NULL)
    {
      fprintf (stderr, "Invalid configuration file.\n");
      xmlFreeDoc (doc);
    }
  cur = cur->xmlChildrenNode;
  while (cur)
    {
      if (!xmlStrcmp (cur->name, "db"))
	db_config (db, cur);
      cur = cur->next;
    }  
  xmlFreeDoc (doc);
}

void
usage (const char *progname)
{
  fprintf (stderr, "Usage:  %s [-c CATEGORY] [-C CONFIG-FILE] ROOT\n",
	   progname);
  exit (EXIT_FAILURE);
}

int
main (int argc, char *argv[])
{
  char path[1024], find_cmd[1024], opt;
  char *category = NULL, *config_file = "prs.conf";
  FILE *fp;
  FileInfo *i;
  Database *db = db_new ();
  int recording_table_created, user_table_created;

  while ((opt = getopt (argc, argv, "c:C:")) != -1)
    {
      switch (opt)
	{
	case 'c':
	  category = optarg;
	  break;
	case 'C':
	  config_file = optarg;
	  break;
	case '?':
	default:
	  usage (argv[0]);
	  break;
	}
    }

  if (argc - optind != 1)
    usage (argv[0]);

  config (db, config_file);

  /* Create list of audio files */

  nice (20);
  sprintf (find_cmd, "find \"%s\" -type f -print", argv[optind]);
  fp = popen (find_cmd, "r");
  
  /* Create tables */

  if (!check_recording_tables (db))
    {
      recording_table_created = 1;
      create_recording_tables (db);
    }
  else
    recording_table_created = 0;
  if (!check_user_table (db))
    {
      user_table_created = 1;
      create_user_table (db);
    }
  else
    user_table_created = 0;
  if (!check_playlist_tables (db))
    create_playlist_tables (db);
  if (!check_log_table (db))
    create_log_table (db);
  
  /* Loop through all files found */

  while (!feof (fp))
    {
      Recording *r = NULL;

      fgets (path, 1024, fp);
      if (feof (fp))
	break;
      path[strlen (path) - 1] = 0;
      if (!recording_table_created)
	{
	  i = file_info_new (path, 0, 0);
	  r = find_recording_by_path (db, path);
	  if (r && abs (i->length-r->length) <= .001 &&
	      !strcmp (r->category, category == NULL ? i->genre : category) &&
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
	  i = file_info_new (path, 1000, 2000);
	  r = (Recording *) malloc (sizeof(Recording));
	}
      else
	{
	  i = file_info_new (path, 1000, 2000);
	  r = (Recording *) malloc (sizeof(Recording));
	}
      if (!i)
	      continue;
      if (i->name)
	r->name = strdup (i->name);
      else
	r->name = strdup ("");
      r->path = strdup (i->path);
      if (i->artist)
	r->artist = strdup (i->artist);
      else
	r->artist = strdup ("");
      if (category != NULL)
	r->category = strdup (category);
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
      add_recording (r, db);
      recording_free (r);
      file_info_free (i);
    }

  pclose (fp);
  db_close (db);
  return 0;
}
