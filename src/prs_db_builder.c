/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

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
      return -1;
    }
  cur = cur->xmlChildrenNode;
  while (cur)
    {
      if (!xmlStrcasecmp (cur->name, (xmlChar*)"db"))
	db_config (db, cur);
      cur = cur->next;
    }  
  xmlFreeDoc (doc);
  return 0;
}

static void
usage (const char *progname)
{
  fprintf (stderr, "Usage:  %s [-c CATEGORY] [-C CONFIG-FILE] ROOT\n",
	   progname);
  exit (EXIT_FAILURE);
}

static void
verify_database (Database *db)
{
	list *recordings, *tmp;
	Recording *r;
	
	fprintf (stderr, "Verifying database...\n");
	recordings = get_recordings (db);
	tmp = recordings;
	while (tmp) {
		FILE *fp;
		r = (Recording *) tmp->data;
		fp = fopen (r->path, "rb");
		if (!fp) {
			delete_recording (r);
		}
		else
			fclose (fp);
	tmp = tmp->next;
	}
	recording_list_free (recordings);
	fprintf (stderr, "Done verifying database.\n");
}


int
main (int argc, char *argv[])
{
  char path[1024], find_cmd[1024];
  char *category = NULL, *config_file = "prs.conf";
  FILE *fp;
  FileInfo *i;
  Database *db = db_new ();
  int opt, recording_table_created, user_table_created;

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

  if (config (db, config_file) == -1)
    {
      return 1;
    }

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
  
  /* Verify all file paths in the database */

  verify_database (db);

  /* Create list of audio files */

  nice (20);
  sprintf (find_cmd, "find \"%s\" -type f -print", argv[optind]);
  fp = popen (find_cmd, "r");
  
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
	  r = find_recording_by_path (db, path);
	  if (r)
	    {
	      recording_free (r);
	      continue;
	    }
	}
      i = file_info_new (path, 1000, 2000);
      r = (Recording *) malloc (sizeof(Recording));
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
