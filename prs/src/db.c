/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * db.c: Implementation of the PRS interface to the MySQL database.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <mysql/mysql.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "debug.h"
#include "db.h"



Database *
db_new (void)
{
	Database *db = NULL;
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "creating database object\n");
	db = (Database *) malloc (sizeof (Database));
	assert (db != NULL);
	db->conn = mysql_init (NULL);
	assert (db->conn != NULL);
	pthread_mutex_init (&(db->mutex), NULL);
	return db;
}

static void
db_lock (Database *db)
{
	assert (db != NULL);
	pthread_mutex_lock (&(db->mutex));
}

static void
db_unlock (Database *db)
{
	assert (db != NULL);
	pthread_mutex_unlock (&(db->mutex));
}

int
db_connect (Database *db, const char *host, const char *user,
	    const char *password, const char *name)
{
	MYSQL *result;
	assert (db != NULL);
	assert (host != NULL);
	assert (user != NULL);
	assert (password != NULL);
	assert (name != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "db_connect: host=%s, user=%s, apssword=%s, name=%s\n",
		      host, user, password, name);
	db_lock (db);
	result = mysql_real_connect (db->conn, host, user, password, name, 0,
				     NULL, 0);
	db_unlock (db);

	if (result)
		return 0;
	else
	{
		fprintf (stderr, "Unable to connect to database: %s\n",
			 mysql_error (db->conn));
		db_close (db);
		exit (EXIT_FAILURE);
	}
}

static void
db_execute (Database *db, const char *stmt)
{
	assert (db != NULL);
	assert (stmt != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "db: executing: %s\n", stmt);
	if (mysql_real_query (db->conn, stmt, strlen (stmt)) != 0) {
		fprintf (stderr, "MySQL error: %s\n", mysql_error (db->conn));
		exit (EXIT_FAILURE);
	}
}

static MYSQL_RES *
db_query (Database *db, const char *query)
{
	MYSQL_RES *res = NULL;
	assert (db != NULL);
	assert (query != NULL);
	db_execute (db, query);
	res = mysql_store_result (db->conn);
	assert (res != NULL);
	return res;
}

void
db_close (Database *db)
{
	assert (db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE, "closing database\n");
	db_lock (db);
	if (db->conn)
		mysql_close (db->conn);
	db_unlock (db);
	free (db);
}

void
db_config (Database *db, xmlNodePtr cur)
{
	xmlChar *host = NULL, *user = NULL, *password = NULL, *name = NULL;
	host = xmlGetProp (cur, "host");
	if (host == NULL)
		host = "localhost";
	user = xmlGetProp (cur, "user");
	password = xmlGetProp (cur, "password");
	name = xmlGetProp (cur, "name");
	if (name == NULL)
		name = "prs";
	db_connect (db, host, user, password, name);
}



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
	assert (new_string != NULL);
  
	while (s && *s)
	{
		if (*s == '\'')
			*ptr++ = '\\';
		*ptr++ = *s++;
	}
	*ptr = 0;
	return new_string;
}



static int
create_table (Database *db,
	      const char *table_name,
	      const char *create_query,
	      int erase_existing)
{
	MYSQL_RES *res;
	char query[2048];
  
	assert (db != NULL);
	assert (table_name != NULL);
	assert (create_query != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "create_table: name=%s, erase_existing=%d\n",
		      table_name, erase_existing);

	/* Does table already exist */

	sprintf (query, "describe %s", table_name);
	if (mysql_real_query (db->conn, query, strlen (query)) == 0)
	{
		debug_printf (DEBUG_FLAGS_DATABASE, "table exists\n");
		res = mysql_store_result (db->conn);
		mysql_free_result (res);
		if (!erase_existing)
			return -1;
		sprintf (query, "drop table %s", table_name);
		db_execute (db, query);
	}

	db_execute (db, create_query);
	return 0;
}



static int
does_table_exist (Database *db,
		  const char *table_name)
{
	MYSQL_RES *res;
	char query[1024];
	int rv;

	assert (db != NULL);
	assert (table_name != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "checking whether table %s exists\n", table_name);
	sprintf (query, "describe %s", table_name);

	if (mysql_real_query (db->conn, query, strlen (query)) == 0)
	{
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "table %s exists\n", table_name);
		res = mysql_store_result (db->conn);
		mysql_free_result (res);
		rv = 1;
	}
	else {
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "table %s does not exist\n", table_name);
		rv = 0;
	}
	return rv;
}



static Recording *
get_recording_from_result (Database *db,
			   MYSQL_ROW row)
{
	Recording *r = NULL;
	assert (db != NULL);
	assert (row != NULL);
	r = (Recording *) malloc (sizeof(Recording));
	assert (r != NULL);
	r->db = db;

	/* Get recording information from database result */

	r->id = atoi (row[0]);
	r->name = strdup (row[1]);
	r->path = strdup (row[2]);
	r->artist = strdup (row[3]);
	r->category = strdup (row[4]);
	r->date = strdup (row[5]);
	r->rate = atoi (row[6]);
	r->channels = atoi (row[7]);
	r->length = atof (row[8]);
	r->audio_in = atof (row[9]);
	r->audio_out = atof (row[10]);
	return r;
}



static PlaylistEvent *
get_playlist_event_from_result (MYSQL_ROW row)
{
	PlaylistEvent *e = NULL;
	char *type;

	assert (row != NULL);
	e = (PlaylistEvent *) malloc (sizeof(PlaylistEvent));
	assert (e != NULL);
	e->template_id = atoi (row[0]);
	e->number = atoi (row[1]);
	type = row[3];
	if (!strcmp (type, "simple_random"))
		e->type = EVENT_TYPE_SIMPLE_RANDOM;
	else if (!strcmp (type, "random"))
		e->type = EVENT_TYPE_RANDOM;
	else if (!strcmp (type, "fade"))
		e->type = EVENT_TYPE_FADE;
	else if (!strcmp (type, "url"))
		e->type = EVENT_TYPE_URL;
	else if (!strcmp (type, "path"))
		e->type = EVENT_TYPE_PATH;
	else
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "unknown event type %s\n", type);

	e->channel_name = strdup (row[4]);
	e->level = atof (row[5]);
	e->anchor_event_number = atoi (row[6]);
	e->anchor_position = atoi (row[7]);
	e->offset = atof (row[8]);
	e->detail1 = strdup (row[9]);
	e->detail2 = strdup (row[10]);
	e->detail3 = strdup (row[11]);
	e->detail4 = strdup (row[12]);
	e->detail5 = strdup (row[13]);
	e->start_time = e->end_time = -1.0;
	return e;
}



static void
playlist_event_list_free (list *l)
{
	list *tmp;

	debug_printf (DEBUG_FLAGS_DATABASE,
		      "playlist_event_list_free called\n");
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
get_playlist_events_from_template (Database *db,
				   int template_id)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char buffer[1024];
	list *events = NULL;
  
	assert (db != NULL);
	assert (template_id >= 0);
	sprintf (buffer, "select * from playlist_event where template_id = %d order by event_number desc", template_id);
	res = db_query (db, buffer);
	while (row = mysql_fetch_row (res))
	{
		PlaylistEvent *e = get_playlist_event_from_result (row);
		events = list_prepend (events, (void *) e);
	}

	mysql_free_result (res);
	return events;
}



int
check_recording_tables (Database *db)
{
	int rv;

	assert (db != NULL);
	db_lock (db);
	rv = (does_table_exist (db, "artist") &&
	      does_table_exist (db, "category") &&
	      does_table_exist (db, "recording"));
	db_unlock (db);
	return rv;
}



void
create_recording_tables (Database *db)
{
	char *artist_create_query = 
		"create table artist (
      artist_id int primary key auto_increment,
      artist_name varchar (200))";
	char *category_create_query = 
		"create table category (
      category_id int primary key auto_increment,
      category_name varchar (128))";
	char *recording_create_query =
		"create table recording (
      recording_id int primary key auto_increment,
      recording_name varchar (255),
      recording_path varchar (255),
      artist_id int,
      category_id int,
      date varchar (20),
      rate int,
      channels int,
      length double,
      audio_in double,
      audio_out double)";

	assert (db != NULL);
	db_lock (db);
  
	create_table (db,
		      "artist",
		      artist_create_query,
		      1);
	create_table (db,
		      "category",
		      category_create_query,
		      1);
	create_table (db,
		      "recording",
		      recording_create_query,
		      1);
	db_unlock (db);
}



void
recording_free (Recording *r)
{
	assert (r != NULL);
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
check_playlist_tables (Database *db)
{
	int rv;

	assert (db != NULL);
	db_lock (db);
	rv = (does_table_exist (db, "playlist_template") &&
	      does_table_exist (db, "playlist_event") &&
	      does_table_exist (db, "schedule"));
	db_unlock (db);
	return rv;
}




void
create_playlist_tables (Database *db)
{
	char *playlist_template_create_query =
		"create table playlist_template (
      template_id int primary key auto_increment,
      template_name varchar (100),
      repeat_events int,
      handle_overlap int,
      artist_exclude double,
      recording_exclude double)";
	char *create_schedule_query =
		"create table schedule (
    time_slot_id int primary key auto_increment,
    start_time double,
    length double,
    repetition double,
    daylight int,
    template_id int,
    fallback_id int,
    end_prefade double)";

	char *create_playlist_event_query =
		"create table playlist_event (
      template_id int,
      event_number int,
      event_name varchar (100),
      event_type varchar (20),
      event_channel_name varchar (100),
      event_level double,
      event_anchor_event_number int,
      event_anchor_position int,
      event_offset double,
      detail1 varchar (64),
      detail2 varchar (64),
      detail3 varchar (64),
      detail4 varchar (64),
      detail5 varchar (64))";

	assert (db != NULL);
	db_lock (db);
	create_table (db,
		      "playlist_template",
		      playlist_template_create_query,
		      1);
	create_table (db,
		      "schedule",
		      create_schedule_query,
		      1);
	create_table (db,
		      "playlist_event",
		      create_playlist_event_query,
		      1);
	db_unlock (db);
}



void
playlist_template_destroy (PlaylistTemplate *t)
{
	assert (t != NULL);
	if (t->name)
		free (t->name);
	if (t->events)
		playlist_event_list_free (t->events);
	free (t);
}



static PlaylistTemplate *
get_playlist_template_from_result (Database *db,
				   MYSQL_ROW row)
{
	PlaylistTemplate *t = NULL;

	assert (db != NULL);
	assert (row != NULL);
	t = (PlaylistTemplate *) malloc (sizeof(PlaylistTemplate));
	assert (t != NULL);
	t->id = atoi (row[0]);
	t->name = strdup (row[1]);
	t->repeat_events = atoi (row[2]);
	t->handle_overlap = atoi (row[3]);
	t->artist_exclude = atof (row[4]);
	t->recording_exclude = atof (row[5]);
	t->type = TEMPLATE_TYPE_STANDARD;
	t->events = get_playlist_events_from_template (db, t->id);
	t->db = db;
	return t;
}



static int
is_daylight (double cur_time)
{
	time_t ct;
	struct tm *timestruct;

	ct = (time_t) cur_time;
	timestruct = localtime(&ct);
	return timestruct->tm_isdst;
}


PlaylistTemplate *
get_playlist_template (Database *db, double cur_time)
{
	PlaylistTemplate *t = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char buffer[1024];
	int daylight;
	double repetition;
	char *schedule_query =
    "select schedule.template_id, template_name, repeat_events, handle_overlap,
    artist_exclude, recording_exclude, start_time,
    length, repetition, fallback_id,
    end_prefade from playlist_template, schedule
    where playlist_template.template_id = schedule.template_id and
    ((repetition = 0 and start_time <= %lf and start_time+length > %lf) or
    (repetition != 0 and start_time <= %lf and mod(%lf-start_time-(daylight*3600)+%d, repetition) < length))
    order by start_time desc";

	daylight = is_daylight (cur_time);
	assert (db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "get_playlist_template: cur_time=%f\n", cur_time);
	db_lock (db);
	sprintf (buffer, schedule_query, cur_time, cur_time, cur_time, cur_time, daylight*3600);
	res = db_query (db, buffer);
	if (mysql_num_rows (res) < 1 ||
	    (row = mysql_fetch_row (res)) == NULL)
	{
		mysql_free_result (res);
		db_unlock (db);
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "no template found\n");
		return NULL;
	}
	t = get_playlist_template_from_result (db, row);
	t->start_time = atof (row[6]);
	t->end_time = atof (row[7]);
	repetition = atof (row[8]);

	if (repetition != 0) {
		
		/* Figure out th start and nd times of the current
		 * repeition of this template
		 *
		 */

		t->start_time = cur_time -
			(int) (cur_time-t->start_time-is_daylight(t->start_time)*3600+daylight*3600)%(int) repetition;
	}
	t->end_time += t->start_time;
	
	t->fallback_id = atoi (row[8]);
	t->end_prefade = atof(row[9]);;
	mysql_free_result (res);
	db_unlock (db);
  
	return t;
}



PlaylistTemplate *
get_playlist_template_by_id (Database *db, int id)
{
	PlaylistTemplate *t;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char buffer[1024];

	assert (db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "get_playlist_template_by_id: id=%d\n", id);

	db_lock (db);
	sprintf (buffer, "select template_id, template_name, repeat_events, handle_overlap, artist_exclude, recording_exclude from playlist_template where template_id = %d", id);
	res = db_query (db, buffer);
	if (mysql_num_rows (res) != 1 ||
	    (row = mysql_fetch_row (res)) == NULL)
	{
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "no template found\n");
		mysql_free_result (res);
		db_unlock (db);
		return NULL;
	}
	t = get_playlist_template_from_result (db, row);
	t->start_time = -1;
	t->end_time = -1;
	t->fallback_id = -1;
	t->end_prefade = -1.0;
	mysql_free_result (res);
	db_unlock (db);
  
	return t;
}



void
playlist_event_free (PlaylistEvent *e)     
{
	assert (e != NULL);
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
  
	assert (t != NULL);
	assert (t->events != NULL);
	e = (PlaylistEvent *) list_get_item (t->events, event_number-1);
	return e;
}




int
check_user_table (Database *db)
{
	int rv;

	assert (db != NULL);
	db_lock (db);
	rv = does_table_exist (db, "prs_user");
	db_unlock (db);
	return rv;
}



void
create_user_table (Database *db)
{
	char *prs_users_create_query =
		"create table prs_user (
      user_id int primary key auto_increment,
      user_name varchar (20),
      password varchar (20),
      email varchar (255),
      type varchar (20))";

	assert (db != NULL);
	db_lock (db);
	create_table (db,
		      "prs_user",
		      prs_users_create_query,
		      1);
	db_unlock (db);
}



void
add_recording (Recording *r, Database *db)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
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
      (recording_name,
      recording_path, artist_id,
      category_id, date,
      rate, channels,
      length, audio_in,
       audio_out) values (
      '%s', '%s', %d, %d, '%s',
      %d, %d, %lf, %lf, %lf)";
      
	assert (r != NULL);
	assert (db != NULL);
	db_lock (db);
	recording_name = process_for_sql (r->name);
	recording_path = process_for_sql (r->path);
	artist_name = process_for_sql (r->artist);
	category_name = process_for_sql (r->category);
	recording_date = process_for_sql (r->date);
	sprintf (buffer, "select artist_id from artist where artist_name = '%s'",
		 artist_name);
	res = db_query (db, buffer);
	if (mysql_num_rows (res) == 1 &&
	    (row = mysql_fetch_row (res)) != NULL)
	{
		artist_id = atoi (row[0]);
		mysql_free_result (res);
	}
	else
	{
		mysql_free_result (res);
		sprintf (buffer, "insert into artist (artist_name) values ('%s')", 
			 artist_name);
		db_execute (db, buffer);
		artist_id = mysql_insert_id (db->conn);
	}
	sprintf (buffer,
		 "select category_id from category where category_name = '%s'",
		 category_name);
	res = db_query (db, buffer);
	if (mysql_num_rows (res) == 1 &&
	    (row = mysql_fetch_row (res)) != NULL)
	{
		category_id = atoi (row[0]);
		mysql_free_result (res);
	}
	else
	{
		mysql_free_result (res);
		sprintf (buffer, "insert into category (category_name) values ('%s')",
			 category_name);
		db_execute (db, buffer);
		category_id = mysql_insert_id (db->conn);
	}

	sprintf (buffer, recording_insert_query, recording_name, recording_path,
		 artist_id, category_id, recording_date, r->rate, r->channels,
		 r->length, r->audio_in, r->audio_out);
	db_execute (db, buffer);
	r->db = db;
	r->id = mysql_insert_id (db->conn);
	free (recording_name);
	free (recording_path);
	free (artist_name);
	free (category_name);
	free (recording_date);
	db_unlock (db);
}



void
delete_recording (Recording *r)
{
	char buffer[1024];

	assert (r != NULL);
	assert (r->db != NULL);
	db_lock (r->db);
	sprintf (buffer, "delete from recording where recording_id = %d", r->id);
	db_execute (r->db, buffer);
	db_unlock (r->db);
	r->db = NULL;
}



Recording *
find_recording_by_path (Database *db, const char *path)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
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

	assert (db != NULL);
	assert (path != NULL);
	db_lock (db);
	recording_path = process_for_sql (path);
	sprintf (buffer, "%s and recording.recording_path = '%s'", select_query,
		 recording_path);
	res = db_query (db, buffer);
	if (mysql_num_rows (res) != 1 ||
	    (row = mysql_fetch_row (res)) == NULL) {
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "recording with path %s not found\n", path);
		r = NULL;
	} else
		r = get_recording_from_result (db, row);
	mysql_free_result (res);
	db_unlock (db);
	return r;
}



list *
get_recordings (Database *db)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	Recording *r;
	char buffer[1024];
	int i = 0;
	list *rv = NULL;
	

	if (!db)
		return NULL;
	db_lock (db);
	sprintf (buffer, "select * from recording");
	if (mysql_real_query (db->conn, buffer, strlen(buffer)) < 0 ||
	    (res = mysql_store_result (db->conn)) == NULL) {
		db_unlock (db);
		return NULL;
	}
	i = mysql_num_rows (res);
	while (i) {
		row = mysql_fetch_row (res);
		r = get_recording_from_result (db, row);
		rv = list_prepend (rv, r);
		i--;
	}
	mysql_free_result (res);
	db_unlock (db);
	return rv;
}


RecordingPicker *
recording_picker_new (Database *db, double artist_exclude,
		      double recording_exclude)
{
	static int randomized = 0;
	char buffer[1024];
	int i;
	RecordingPicker *p = NULL;

	assert (db != NULL);
	p = (RecordingPicker *) malloc (sizeof (RecordingPicker));
	assert (p != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "recording_picker_new: artist_exclude=%d, "
		      "recording_exclude=%d\n", artist_exclude,
		      recording_exclude);
	db_lock (db);
  
	if (!randomized)
	{
		srand (time (NULL));
		randomized = 1;
	}
	p->artist_exclude_table_name = strdup ("artist_exclude");
	p->recording_exclude_table_name = strdup ("recording_exclude");

	/* Create the tables */

	sprintf (buffer, "create table %s (recording_id int, time double)",
		 p->recording_exclude_table_name);
	create_table (db,
		      p->recording_exclude_table_name,
		      buffer,
		      0);
	sprintf (buffer, "create table %s (artist_name varchar(200), time double)",
		 p->artist_exclude_table_name);
	create_table (db,
		      p->artist_exclude_table_name,
		      buffer,
		      0);
	p->recording_exclude = recording_exclude;
	p->artist_exclude = artist_exclude;
	p->db = db;
	db_unlock (db);
	return p;
}



void
recording_picker_destroy (RecordingPicker *p)
{
	char buffer[1024];
  
	assert (p != NULL);
	assert (p->db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "recording_picker_destroy called\n");

	db_lock (p->db);
	db_unlock (p->db);
	free (p);
}



Recording *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 double cur_time)
{
	list *tmp;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char *buffer;
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
	time_t ct;
    
	assert (p != NULL);
	assert (p->db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "recording_picker_select: cur_time=%f\n", cur_time);

	db_lock (p->db);
	ct = cur_time;
	*category_part = 0;
	for (tmp = category_list; tmp; tmp = tmp->next) {
		char temp[256];
		char *s = (char *) tmp->data;
		if (tmp->prev)
			sprintf (temp, " or category_name = '%s'", s);
		else
			sprintf (temp, " and (category_name = '%s'", s);
		strcat (category_part, temp);
	}
	if (category_list)
		strcat (category_part, ")");
	if (cur_time >= 0) {
		list *artists = NULL, *recordings = NULL;
		int artists_strlen = 0, recordings_strlen = 0, first;
		char temp[1024];

		/* Delete old items from exclude tables */

		sprintf (temp, "delete from %s where %s.time < %ld",
			 p->recording_exclude_table_name,
			 p->recording_exclude_table_name,
			 (long) (cur_time-p->recording_exclude));
		db_execute (p->db, temp);
		sprintf (temp, "delete from %s where %s.time < %ld",
			 p->artist_exclude_table_name,
			 p->artist_exclude_table_name,
			 (long) (cur_time-p->artist_exclude));
		db_execute (p->db, temp);
		sprintf (temp, "select artist_name from %s",
			 p->artist_exclude_table_name);
		res = db_query (p->db, temp);
		first = 1;

		while (row = mysql_fetch_row (res))
		{
			if (first)
				first = 0;
			else
				artists_strlen += 2;

			artists = string_list_append (artists, row[0]);
			artists_strlen += strlen (row[0]);
		}

		mysql_free_result (res);
		sprintf (temp, "select recording_id from %s",
			 p->recording_exclude_table_name);
		res = db_query (p->db, temp);
		first = 1;

		while (row = mysql_fetch_row (res))
		{
			if (first)
				first = 0;
			else
				recordings_strlen += 2;

			recordings = string_list_append (recordings, row[0]);
			recordings_strlen += strlen (row[0]);
		}

		mysql_free_result (res);
		buffer = malloc (artists_strlen + recordings_strlen + 2048);
		assert (buffer != NULL);
		strcpy (buffer, select_query);
		if (category_list)
			strcat (buffer, category_part);
		strcat (buffer, " and recording.recording_id not in (");
		first = 1;

		for (tmp = recordings; tmp; tmp = tmp->next)
		{
			if (first)
				first = 0;
			else
				strcat (buffer, ", ");

			strcat (buffer, tmp->data);
		}

		if (first)
			strcat (buffer, "null");
		strcat (buffer, ") and artist_name not in (");
		first = 1;

		for (tmp = artists; tmp; tmp = tmp->next)
		{
			char *tmp_str = process_for_sql (tmp->data);

			if (first)
				first = 0;
			else
				strcat (buffer, ", ");

			strcat (buffer, "'");
			strcat (buffer, tmp_str);
			strcat (buffer, "'");
			free (tmp_str);
		}

		if (first)
			strcat (buffer, "null");
		strcat (buffer, ")");
		string_list_free (artists);
		string_list_free (recordings);
	}
	else
	{
		buffer = malloc (2048);
		assert (buffer != NULL);
		strcpy (buffer, select_query);
		if (category_list)
			strcat (buffer, category_part);
	}
	res = db_query (p->db, buffer);
	if (mysql_num_rows (res) <= 0)
	{
		mysql_free_result (res);
		free (buffer);
		db_unlock (p->db);
		debug_printf (DEBUG_FLAGS_DATABASE, "no recording found\n");
		return NULL;
	}

	n = rand () % mysql_num_rows (res);
	mysql_data_seek (res, n);
	row = mysql_fetch_row (res);

	if (row == NULL)
	{
		mysql_free_result (res);
		free (buffer);
		db_unlock (p->db);
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "row fetched after mysql_data_seek is NULL\n");
		return NULL;
	}

	r = get_recording_from_result (p->db, row);
	mysql_free_result (res);

	/* Add info to the exclude tables */

	if (cur_time >= 0)
	{
		char *artist_name = process_for_sql (r->artist);
		sprintf (buffer, "insert into %s values (%d, %lf)",
			 p->recording_exclude_table_name, r->id, cur_time);
		db_execute (p->db, buffer);
		sprintf (buffer, "insert into %s values ('%s', %lf)",
			 p->artist_exclude_table_name, artist_name, cur_time);
		db_execute (p->db, buffer);
		free (artist_name);
	}  

	free (buffer);
	db_unlock (p->db);
	return r;
}



int
check_log_table (Database *db)
{
	int rv;

	assert (db != NULL);
	db_lock (db);
	rv = does_table_exist (db, "log");
	db_unlock (db);
	return rv;
}



void
create_log_table (Database *db)
{
	char *log_create_query =
		"create table log (
      recording_id int,
      start_time int,
      length int)";

	assert (db != NULL);
	db_lock (db);
	create_table (db,
		      "log",
		      log_create_query,
		      1);  
	db_unlock (db);
}



void
add_log_entry (Database *db, int recording_id,
	       int start_time,
	       int length)
{
	char buffer[1024];

	assert (db != NULL);
	db_lock (db);
	sprintf (buffer, "insert into log values (%d, %d, %d)", recording_id,
		 start_time, length);
	db_execute (db, buffer);
	db_unlock (db);
}
