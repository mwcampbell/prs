#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <libpq-fe.h>
#include "db.h"



/*
 *
 * Global data
 *
 */



static PGconn *connection = NULL;




/*
 *
 * Static functions
 *
 */



static char *
process_for_sql (const char *s)
{
  char *new_string;
  char *ptr;
  
  if (s)
    ptr = new_string = malloc (strlen(s)*2+1);
  else
    ptr = new_string = malloc (1);
  
  while (s && *s)
    {
      if (*s == '\'')
	*ptr++ = '\'';
      *ptr++ = *s++;
    }
  *ptr = 0;
  return new_string;
}



static int
create_table (const char *table_name,
	      const char *create_query,
	      list *read_only_access_list,
	      list *total_access_list,
	      int erase_existing)
{
  PGresult *res;
  char query[2048];
  list *tmp;
  
  if (!connection)
    return -1;

  /* Does table already exist */

  sprintf (query, "select * from %s;", table_name);
  res = PQexec (connection, query);
  if (PQresultStatus (res) == PGRES_TUPLES_OK)
    {
      PQclear (res);
      if (!erase_existing)
	return -1;
      sprintf (query, "drop table %s;", table_name);
      res = PQexec (connection, query);
      if (PQresultStatus (res) != PGRES_COMMAND_OK)
	{
	  PQclear (res);
	  return -1;
	}
    }
  else
    PQclear (res);
  res = PQexec (connection, create_query);
  if (PQresultStatus (res) != PGRES_COMMAND_OK)
    {
      PQclear (res);
      return -1;
    }
  PQclear (res);


  /* Grant read-only access */

  if (read_only_access_list)
    {
      for (tmp = read_only_access_list; tmp; tmp = tmp->next)
	{
	  char *user = (char *) tmp->data;
	  if (!user)
	    continue;
	  sprintf (query, "grant select on %s to %s;", table_name, user);
	  res = PQexec (connection, query);
	}
    }

  /* Grant total access */

  if (total_access_list)
    {
      for (tmp = total_access_list; tmp; tmp = tmp->next)
	{
	  char *user = (char *) tmp->data;
	  if (!user)
	    continue;
	  sprintf (query, "grant all on %s to %s;\n", table_name, user);
	  res = PQexec (connection, query);
	}
    }

  return 0;
}



static int
does_table_exist (const char *table_name)
{
  PGresult *res;
  char query[1024];
  int rv;
  
  sprintf (query, "select * from %s;", table_name); 
  res = PQexec (connection, query);  
  if (PQntuples (res) > 0)
    rv = 1;
  else
    rv = 0;
  PQclear (res);
  return rv;
}



static Recording *
get_recording_from_result (PGresult *res,
			   int row)
{
  Recording *r = (Recording *) malloc (sizeof(Recording));
  
  if (!r)
    return NULL;

  /* Get recording information from database result */

  r->id = atoi (PQgetvalue (res, row, 0));
  r->name = strdup (PQgetvalue (res, row, 1));
  r->path = strdup (PQgetvalue (res, row, 2));
  r->artist = strdup (PQgetvalue (res, row, 3));
  r->category = strdup (PQgetvalue (res, row, 4));
  r->date = strdup (PQgetvalue (res, row, 5));
  r->rate = atoi (PQgetvalue (res, row, 6));
  r->channels = atoi (PQgetvalue (res, row, 7));
  r->length = atof (PQgetvalue (res, row, 8));
  r->audio_in = atof (PQgetvalue (res, row, 9));
  r->audio_out = atof (PQgetvalue (res, row, 10));
  return r;
}



static PlaylistEvent *
get_playlist_event_from_result (PGresult *res,
			       int row)
{
  PlaylistEvent *e = (PlaylistEvent *) malloc (sizeof(PlaylistEvent));
  char *type;
 
  e->template_id = atoi (PQgetvalue (res, row, 0));
  e->number = atoi (PQgetvalue (res, row, 1));
  type = PQgetvalue (res, row, 3);
  if (!strcmp (type, "simple_random"))
    e->type = EVENT_TYPE_SIMPLE_RANDOM;
  else if (!strcmp (type, "random"))
    e->type = EVENT_TYPE_RANDOM;
  else if (!strcmp (type, "fade"))
    e->type = EVENT_TYPE_FADE;

  e->channel_name = strdup (PQgetvalue (res, row, 4));
  e->level = atof (PQgetvalue (res, row, 5));
  e->anchor_event_number = atoi (PQgetvalue (res, row, 6));
  e->anchor_position = atoi (PQgetvalue (res, row, 7));
  e->offset = atof (PQgetvalue (res, row, 8));
  e->detail1 = strdup (PQgetvalue (res, row, 9));
  e->detail2 = strdup (PQgetvalue (res, row, 10));
  e->detail3 = strdup (PQgetvalue (res, row, 11));
  e->detail4 = strdup (PQgetvalue (res, row, 12));
  e->detail5 = strdup (PQgetvalue (res, row, 13));
  e->start_time = e->end_time = -1.0;
  return e;
}



static void
playlist_event_list_free (list *l)
{
  list *tmp;

  if (!l)
    return;
  for (tmp = l; tmp; tmp = tmp->next)
    {
      if (tmp->data)
	playlist_event_free ((PlaylistEvent *)tmp->data);
    }
  list_free (l);
}



static list *
get_playlist_events_from_template (int template_id)
{
  PGresult *res;
  int i;
  char buffer[1024];
  list *events = NULL;
  
  if (template_id < 0)
    return NULL;
  sprintf (buffer, "select * from playlist_event where template_id = %d order by event_number;", template_id);
  res = PQexec (connection, buffer);
  if (PQntuples (res) <= 0)
    {
      PQclear (res);
      return NULL;
    }
  i = PQntuples (res)-1;
  while (i >= 0)
    {
      PlaylistEvent *e = get_playlist_event_from_result (res, i);
      events = list_prepend (events, (void *) e);
      i--;
    }
  PQclear (res);
  return events;
}



int
check_recording_tables (void)
{
  return (does_table_exist ("artist") &&
	  does_table_exist ("category") &&
	  does_table_exist ("recording"));
}



void
create_recording_tables (list *read_only_users,
			list *total_access_users)
{
  char *artist_create_query = 
    "create table artist (
      artist_id int primary key,
      artist_name varchar (200));";
  char *category_create_query = 
    "create table category (
      category_id int primary key,
      category_name varchar (128));";
  char *recording_create_query =
    "create table recording (
      recording_id int primary key,
      recording_name varchar (256),
      recording_path varchar (1024),
      artist_id int references artist,
      category_id int references category,
      date varchar (20),
      rate int,
      channels int,
      length float,
      audio_in float,
      audio_out float);";
  create_table ("artist",
		artist_create_query,
		read_only_users,
		total_access_users,
		1);
  create_table ("category",
		category_create_query,
		read_only_users,
		total_access_users,
		1);
  create_table ("recording",
		recording_create_query,
		read_only_users,
		total_access_users,
		1);
}



void
recording_free (Recording *r)
{
  if (!r)
    return;
  if (r->name)
    free (r->name);
  if (r->path)
    free (r->path);
  if (r->artist)
    free (r->artist);
  if (r->date)
    free (r->date);
  if (r->category)
    free (r->category);
  free (r);
}



void
recording_list_free (list *l)
{
  list *tmp;

  if (!l)
    return;
  for (tmp = l; tmp; tmp = tmp->next)
    {
      recording_free ((Recording *)tmp->data);
    }
  list_free (l);
}



int
check_playlist_tables (void)
{
  return (does_table_exist ("playlist_template") && does_table_exist ("playlist_event"));
}
void
create_playlist_tables (list *read_only_users,
				list *total_access_users)
{
  char *playlist_template_create_query =
    "create table playlist_template (
      template_id int primary key,
      template_name varchar (100),
      start_time float,
      end_time float,
      repeat_events int,
      artist_exclude float,
      recording_exclude float);";
  char *create_playlist_event_query =
    "create table playlist_event (
      template_id int references playlist_template,
      event_number int,
      event_name varchar (100),
      event_type varchar (20),
      event_channel_name varchar (100),
      event_level float,
      event_anchor_event_number  int,
      event_anchor_position int,
      event_offset float,
      detail1 varchar (64),
      detail2 varchar (64),
      detail3 varchar (64),
      detail4 varchar (64),
      detail5 varchar (64));";

  create_table ("playlist_template",
		playlist_template_create_query,
		read_only_users,
		total_access_users,
		1);
  create_table ("playlist_event",
	      create_playlist_event_query,
	      read_only_users,
	      total_access_users,
	      1);
}



void
playlist_template_destroy (PlaylistTemplate *t)
{
  if (!t)
    return;
  if (t->name)
    free (t->name);
  if (t->events)
    playlist_event_list_free (t->events);
  free (t);
}



static PlaylistTemplate *
get_playlist_template_from_result (PGresult *res,
				   int row)
{
  PlaylistTemplate *t = (PlaylistTemplate *) malloc (sizeof(PlaylistTemplate));

  if (!t)
    {
      fprintf (stderr, "Something is very broken.\n");
      abort ();
      }
  t->id = atoi (PQgetvalue (res, row, 0));
  t->name = strdup (PQgetvalue (res, row, 1));
  t->start_time = atof (PQgetvalue (res, row, 2));
  t->end_time = atof (PQgetvalue (res, row, 3));
  t->repeat_events = atoi (PQgetvalue (res, row, 4));
  t->artist_exclude = atof (PQgetvalue (res, row, 5));
  t->recording_exclude = atof (PQgetvalue (res, row, 6));
  t->events = get_playlist_events_from_template (t->id);
  return t;
}



PlaylistTemplate *
get_playlist_template (double cur_time)
{
  PlaylistTemplate *t;
  PGresult *res;
  char buffer[1024];
  time_t day_start;
  double time_of_day;
  
  tzset ();
  time_of_day = (time_t) (cur_time-timezone+daylight*3600)%86400;
  day_start = (time_t) cur_time-time_of_day;
  time_of_day = cur_time-day_start;
  
  sprintf (buffer, "select * from playlist_template where start_time <= %lf and end_time > %lf;", time_of_day, time_of_day);
  res = PQexec (connection, buffer);
  if (PQntuples (res) != 1)
    {
      PQclear (res);
      return NULL;
    }
  t = get_playlist_template_from_result (res, 0);
  PQclear (res);

  /* Find the start and end times for this instance of the template */
  
  t->start_time += day_start;
  t->end_time += day_start;
  return t;
}



void
playlist_event_free (PlaylistEvent *e)     
{
  if (!e)
    return;
  if (e->channel_name)
      free (e->channel_name);
  if (e->detail1)
    free (e->detail1);
  if (e->detail2)
    free (e->detail2);
  if (e->detail3)
    free (e->detail3);
  if (e->detail4)
    free (e->detail4);
  if (e->detail5)
    free (e->detail5);
  free (e);
}



PlaylistEvent *
playlist_template_get_event (PlaylistTemplate *t,
			     int event_number)
{
  PlaylistEvent *e;
  
  if (!t)
    return NULL;
  if (!t->events)
    return NULL;
  e = (PlaylistEvent *) list_get_item (t->events, event_number-1);
  return e;
}




int
check_user_table (void)
{
  return does_table_exist ("prs_user");
}



void
create_user_table (list *read_only_users,
		   list *total_access_users)
{
  char *prs_users_create_query =
    "create table prs_user (
      user_id int primary key,
      user_name varchar (20),
      password varchar (20),
      email varchar (1024),
      type varchar (20));";
  create_table ("prs_user",
		prs_users_create_query,
		read_only_users,
		total_access_users,
		1);
}



int
connect_to_database (const char *dbname)
{
  char buffer[1024];

  sprintf (buffer, "dbname = %s", dbname);
  connection = PQconnectdb (buffer);
  if (PQstatus (connection) != CONNECTION_OK)
    return -1;
  else
    return 0;
}



void
disconnect_from_database (void)
{
  PQfinish (connection);
}



void
add_recording (Recording *r)
{
  PGresult *res;
  int recording_id;
  int artist_id;
  int category_id;
  char buffer[2048];
  char *recording_name;
  char *recording_path;
  char *artist_name;
  char *category_name;
  char *recording_date;
  char *recording_insert_query =
    "insert into recording
      (recording_id, recording_name,
      recording_path, artist_id,
      category_id, date,
      rate, channels,
      length, audio_in,
       audio_out) values (
      %d, '%s', '%s', %d, %d, '%s',
      %d, %d, %lf, %lf, %lf);";
      
  if (!r)
    return;

  recording_name = process_for_sql (r->name);
  recording_path = process_for_sql (r->path);
  artist_name = process_for_sql (r->artist);
  category_name = process_for_sql (r->category);
  recording_date = process_for_sql (r->date);

  sprintf (buffer, "select artist_id from artist where artist_name = '%s';", artist_name);
  res = PQexec (connection, buffer);
  if (PQntuples (res) == 1)
    {
      artist_id = atoi (PQgetvalue (res, 0, 0));
      PQclear (res);
    }
  else
    {
      PQclear (res);
      res = PQexec (connection, "select max(artist_id) from artist;");
      artist_id = atoi (PQgetvalue (res, 0, 0))+1;
      PQclear (res);
      sprintf (buffer, "insert into artist values (%d, '%s');", artist_id, artist_name);
      res = PQexec (connection, buffer);
      PQclear (res);
    }
  sprintf (buffer, "select category_id from category where category_name = '%s';", category_name);
  res = PQexec (connection, buffer);
  if (PQntuples (res) == 1)
    {
      category_id = atoi (PQgetvalue (res, 0, 0));
      PQclear (res);
    }
  else
    {
      PQclear (res);
      res = PQexec (connection, "select max(category_id) from category;");
      category_id = atoi (PQgetvalue (res, 0, 0))+1;
      PQclear (res);
      sprintf (buffer, "insert into category values (%d, '%s');", category_id, category_name);
      res = PQexec (connection, buffer);
      PQclear (res);
    }

  res = PQexec (connection, "select max(recording_id) from recording;");
  recording_id = atoi (PQgetvalue (res, 0, 0))+1;
  PQclear (res);
  sprintf (buffer, recording_insert_query,
	 recording_id,
	 recording_name,
	 recording_path,
	 artist_id,
	 category_id,
	 recording_date,
	 r->rate,
	 r->channels,
	 r->length,
	 r->audio_in,
	 r->audio_out);
  res = PQexec (connection, buffer);
  PQclear (res);
  free (recording_name);
  free (recording_path);
  free (artist_name);
  free (category_name);
  free (recording_date);
}



void
delete_recording (Recording *r)
{
  PGresult *res;
  char buffer[1024];

  sprintf (buffer, "delete from recording where recording_id = %d;", r->id);
  res = PQexec (connection, buffer);
  PQclear (res);
}



Recording *
find_recording_by_path (const char *path)
{
  PGresult *res;
  Recording *r;
  char buffer[1024];
  char *recording_path;
  char *select_query = 
    "select recording_id, recording_name, recording_path,
      artist_name, category_name, date,
      rate, channels,
      length, audio_in, audio_out
      from recording, artist, category
      where recording.artist_id = artist.artist_id and
      recording.category_id = category.category_id";

  recording_path = process_for_sql (path);
  sprintf (buffer, "%s and recording.recording_path = '%s';", select_query, recording_path);
  res = PQexec (connection, buffer);
  if (PQntuples (res) <= 0)
    r = NULL;
  else
    r = get_recording_from_result (res, 0);
  PQclear (res);
  return r;
}



RecordingPicker *
recording_picker_new (double artist_exclude,
		      double recording_exclude)
{
  static int randomized = 0;
  char buffer[1024];
  int i;
  RecordingPicker *p = (RecordingPicker *) malloc (sizeof (RecordingPicker));
  PGresult *res;
  
  if (!randomized)
    {
      srand (time (NULL));
      randomized = 1;
    }
  p->artist_exclude_table_name = strdup ("artist_exclude");
  p->recording_exclude_table_name = strdup ("recording_exclude");

  /* Create the tables */

  sprintf (buffer, "create table %s (recording_id int, time float);", p->recording_exclude_table_name);
  create_table (p->recording_exclude_table_name,
		buffer,
		NULL,
		NULL,
		1);
  sprintf (buffer, "create table %s (artist_name varchar(200), time float);", p->artist_exclude_table_name);
  create_table (p->artist_exclude_table_name,
		buffer,
		NULL,
		NULL,
		1);
  p->recording_exclude = recording_exclude;
  p->artist_exclude = artist_exclude;
  return p;
}



void
recording_picker_destroy (RecordingPicker *p)
{
  PGresult *res;
  char buffer[1024];
  
  if (!p)
    return;
  if (p->recording_exclude_table_name)
    {
      sprintf (buffer, "drop table %s;", p->recording_exclude_table_name);
      res = PQexec (connection, buffer);
      PQclear (res);
      free (p->recording_exclude_table_name);
    }
  if (p->artist_exclude_table_name)
    {
      sprintf (buffer, "drop table %s;", p->artist_exclude_table_name);
      res = PQexec (connection, buffer);
      PQclear (res);
      free (p->artist_exclude_table_name);
    }
  free (p);
}



Recording *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 double cur_time)
{
  list *tmp;
  PGresult *res;
  char buffer[2048];
  char category_part[1024];
  int n;
  Recording *r;
  char *select_query = 
    "select recording_id, recording_name, recording_path,
      artist_name, category_name, date,
      rate, channels,
      length, audio_in, audio_out
      from recording, artist, category
      where recording.artist_id = artist.artist_id and
      recording.category_id = category.category_id";

  if (!p)
    return NULL;

  strcpy (buffer, select_query);  
  *category_part = 0;
  for (tmp = category_list; tmp; tmp = tmp->next)
    {
      char temp[256];
      char *s = (char *) tmp->data;
      if (tmp->prev)
	sprintf (temp, " or category_name = '%s'", s);
      else
	sprintf (temp, " and (category_name = '%s'", s);
      strcat (category_part, temp);
    }
  if (category_list)
    {
      strcat (category_part, ")");
      strcat (buffer, category_part);
    }
  if (cur_time >= 0)
    {
      char exclude_part[1024];
      char temp[1024];

      /* Delete old items from exclude tables */

      sprintf (temp, "delete from %s where time < %lf;",
	       p->recording_exclude_table_name, cur_time-p->recording_exclude);
      res = PQexec (connection, temp);
      PQclear (res);
      sprintf (temp, "delete from %s where time < %lf;",
	       p->artist_exclude_table_name, cur_time-p->artist_exclude);
      res = PQexec (connection, temp);
      PQclear (res);
      
      sprintf (exclude_part, " and recording.recording_id not in (select recording_id from %s) and artist_name not in (select artist_name from %s)", p->recording_exclude_table_name, p->artist_exclude_table_name);
      strcat (buffer, exclude_part);
    }
  strcat (buffer, ";");
  res = PQexec (connection, buffer);
  if (PQntuples (res) <= 0)
    {
      PQclear (res);
      return NULL;
    }
  n = rand ()%PQntuples (res);
  r = get_recording_from_result (res, n);
  PQclear (res);

  /* Add info to the exclude tables */

  if (cur_time >= 0)
    {
      char *artist_name = process_for_sql (r->artist);
      sprintf (buffer, "insert into %s values (%d, %lf);", p->recording_exclude_table_name, r->id, cur_time);
      res = PQexec (connection, buffer);
      PQclear (res);
      sprintf (buffer, "insert into %s values ('%s', %lf);", p->artist_exclude_table_name, artist_name, cur_time);
      res = PQexec (connection, buffer);
      PQclear (res);
      free (artist_name);
    }  
  return r;
}



int
check_config_status_tables (void)
{
  return (does_table_exist ("config") && does_table_exist ("status"));
}



void
create_config_status_tables (list *read_only_users,
			     list *total_access_users)
{
char *config_create_query = 
  "create table config (
      config_key varchar (256),
      config_value varchar (256));";
 char *status_create_query = "create table status (
      status_key varchar (256),
status_value varchar (256));";

 create_table ("config",
	      config_create_query,
	      read_only_users,
	      total_access_users,
	      1);
 create_table ("status",
	      status_create_query,
	      read_only_users,
	      total_access_users,
	      1);
}



char *
get_config_value (const char *key)
{
  PGresult *res;
  char buffer[1024];
  char *rv;
  char *processed_key;
  
  processed_key = process_for_sql (key);
  sprintf (buffer, "select config_value from config where config_key = '%s';", processed_key);
  res = PQexec (connection, buffer);
  if (PQntuples (res) != 1)
    {
      PQclear (res);
      return NULL;
    }
  rv = strdup (PQgetvalue (res, 0, 0));
  PQclear (res);
  free (processed_key);
  return rv;
}



void
set_config_value (const char *key,
		  const char *value)
{
  PGresult *res;
  char buffer[1024];
  char *processed_key, *processed_value;
  
  processed_key = process_for_sql (key);
  processed_value = process_for_sql (value);

  sprintf (buffer, "delete from config where config_key = '%s';", processed_key);
  res = PQexec (connection, buffer);
  PQclear (res);
  sprintf (buffer, "insert into config values ('%s', '%s');", processed_key, processed_value);
  res = PQexec (connection, buffer);
  PQclear (res);
  free (processed_key);
  free (processed_value);
}




char *
get_status_value (const char *key)
{
  PGresult *res;
  char buffer[1024];
  char *rv;
  char *processed_key;
  
  processed_key = process_for_sql (key);

  sprintf (buffer, "select status_value from status where status_key = '%s';", processed_key);
  res = PQexec (connection, buffer);
  if (PQntuples (res) != 1)
    {
      PQclear (res);
      return NULL;
    }
  rv = strdup (PQgetvalue (res, 0, 0));
  PQclear (res);
  free (processed_key);
  return rv;
}



void
set_status_value (const char *key,
		  const char *value)
{
  PGresult *res;
  char buffer[1024];
  char *processed_key, *processed_value;
  
  processed_key = process_for_sql (key);
  processed_value = process_for_sql (value);

  sprintf (buffer, "delete from status where status_key = '%s';", processed_key);
  res = PQexec (connection, buffer);
  PQclear (res);
  sprintf (buffer, "insert into status values ('%s', '%s');", processed_key, processed_value);
  res = PQexec (connection, buffer);
  PQclear (res);
  free (processed_key);
  free (processed_value);
}




