/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 * db.c: Implementation of the PRS interface to the SQLite database.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <sqlite.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "debug.h"
#include "db.h"



struct _Database
{
	sqlite *db;
	pthread_mutex_t mutex;
};

typedef struct
{
	Database *db;
	list *l;
} QueryCallbackData;

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

static void
db_execute (Database *db, const char *stmt, sqlite_callback callback,
	    void *udata)
{
	char *err;
	assert (db != NULL);
	assert (stmt != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "db: executing: %s\n", stmt);
	if (sqlite_exec (db->db, stmt, callback, udata, &err) != SQLITE_OK) {
		fprintf (stderr, "SQLite error: %s\n", err);
		exit (EXIT_FAILURE);
	}
}

void
db_close (Database *db)
{
	assert (db != NULL);
	assert (db->db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE, "closing database\n");
	db_lock (db);
	sqlite_close (db->db);
	db_unlock (db);
	free (db);
}

Database *
db_new (void)
{
	Database *db;

	db = (Database *) malloc (sizeof (Database));
	assert (db != NULL);
	db->db = NULL;
	pthread_mutex_init (&(db->mutex), NULL);
	return db;
}

void
db_from_config (xmlNodePtr cur,
		Database *db)
{
	xmlChar *filename = xmlGetProp (cur, "filename");
	char *err;
	if (filename == NULL)
		filename = xmlStrdup ("prs.db");
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "creating database object; filename = %s\n", filename);
	db->db = sqlite_open (filename, 1, &err);
	if (db->db == NULL) {
		fprintf (stderr, "Unable to open database: %s\n", err);
		exit (EXIT_FAILURE);
	}
	xmlFree (filename);
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
			*ptr++ = '\'';
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
	char *err;
	char query[2048];
  
	assert (db != NULL);
	assert (table_name != NULL);
	assert (create_query != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "create_table: name=%s, erase_existing=%d\n",
		      table_name, erase_existing);

	/* Does table already exist */

	sprintf (query, "select count(*) from %s", table_name);
	if (sqlite_exec (db->db, query, NULL, NULL, &err) == SQLITE_OK)
	{
		debug_printf (DEBUG_FLAGS_DATABASE, "table exists\n");
		if (!erase_existing)
			return -1;
		sprintf (query, "drop table %s", table_name);
		db_execute (db, query, NULL, NULL);
	}
	else
		free (err);

	db_execute (db, create_query, NULL, NULL);
	return 0;
}



static int
does_table_exist (Database *db,
		  const char *table_name)
{
	char *err;
	char query[1024];
	int rv;

	assert (db != NULL);
	assert (table_name != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "checking whether table %s exists\n", table_name);
	sprintf (query, "select count(*) from %s", table_name);

	if (sqlite_exec (db->db, query, NULL, NULL, &err) == SQLITE_OK)
	{
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "table %s exists\n", table_name);
		rv = 1;
	}
	else {
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "table %s does not exist\n", table_name);
		free (err);
		rv = 0;
	}
	return rv;
}



static int
recording_query_cb (void *udata, int cols, char **row, char **col_names)
{
	QueryCallbackData *data = (QueryCallbackData *) udata;
	Recording *r = NULL;
	assert (row != NULL);
	r = (Recording *) malloc (sizeof(Recording));
	assert (r != NULL);
	r->db = data->db;

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
	data->l = list_prepend (data->l, r);
	return 0;
}



static int
playlist_event_query_cb (void *udata, int cols, char **row, char **col_names)
{
	list **lp = (list **) udata;
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
	*lp = list_prepend (*lp, e);
	return 0;
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
	char buffer[1024];
	list *events = NULL;
  
	assert (db != NULL);
	assert (template_id >= 0);
	sprintf (buffer, "select * from playlist_event where template_id = %d order by event_number desc", template_id);
	db_execute (db, buffer, playlist_event_query_cb, &events);
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
		"create table artist ("
      "artist_id integer primary key,"
      "artist_name varchar (200))";
	char *category_create_query = 
		"create table category ("
      "category_id integer primary key,"
      "category_name varchar (128))";
	char *recording_create_query =
		"create table recording ("
      "recording_id integer primary key,"
      "recording_name varchar (255),"
      "recording_path varchar (255),"
      "artist_id int,"
      "category_id int,"
      "date varchar (20),"
      "rate int,"
      "channels int,"
      "length double,"
      "audio_in double,"
      "audio_out double)";

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
		"create table playlist_template ("
      "template_id integer primary key,"
      "template_name varchar (100),"
      "repeat_events int,"
      "handle_overlap int,"
      "artist_exclude double,"
      "recording_exclude double)";
	char *create_schedule_query =
		"create table schedule ("
    "time_slot_id integer primary key,"
    "start_time double,"
    "length double,"
    "repetition double,"
    "daylight int,"
    "template_id int,"
    "fallback_id int,"
    "end_prefade double)";

	char *create_playlist_event_query =
		"create table playlist_event ("
      "template_id int,"
      "event_number int,"
      "event_name varchar (100),"
      "event_type varchar (20),"
      "event_channel_name varchar (100),"
      "event_level double,"
      "event_anchor_event_number int,"
      "event_anchor_position int,"
      "event_offset double,"
      "detail1 varchar (64),"
      "detail2 varchar (64),"
      "detail3 varchar (64),"
      "detail4 varchar (64),"
      "detail5 varchar (64))";

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



static int
playlist_template_query_cb (void *udata, int cols, char **row, char **names)
{
	PlaylistTemplate *t = NULL;
	QueryCallbackData *data = (QueryCallbackData *) udata;
	assert (row != NULL);
	t = (PlaylistTemplate *) malloc (sizeof(PlaylistTemplate));
	assert (t != NULL);
	t->id = atoi (row[0]);
	t->name = strdup (row[1]);
	t->repeat_events = atoi (row[2]);
	t->handle_overlap = atoi (row[3]);
	t->artist_exclude = atof (row[4]);
	t->recording_exclude = atof (row[5]);

	if (cols > 6) {
		t->start_time = atof (row[6]);
		t->end_time = t->start_time + atof (row[7]);
		t->repetition = atof (row[8]);
		t->fallback_id = atoi (row[9]);
		t->end_prefade = atof(row[10]);;
	}

	t->type = TEMPLATE_TYPE_STANDARD;
	t->db = data->db;
	data->l = list_prepend (data->l, t);
	return 0;
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
	QueryCallbackData data;
	PlaylistTemplate *t = NULL;
	char buffer[1024];
	int daylight;
	char *schedule_query =
    "select schedule.template_id, template_name, repeat_events, handle_overlap,"
    "artist_exclude, recording_exclude, start_time,"
    "length, repetition, fallback_id,"
    "end_prefade from playlist_template, schedule "
    "where playlist_template.template_id = schedule.template_id and "
    "((repetition = 0 and start_time <= %lf and start_time+length > %lf) or"
    "(repetition != 0 and start_time <= %lf and (%lf-start_time-(daylight*3600)+%d)%(repetition) < length))"
    "order by time_slot_id desc";
	daylight = is_daylight (cur_time);
	assert (db != NULL);
	data.db = db;
	data.l = NULL;
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "get_playlist_template: cur_time=%f\n", cur_time);
	db_lock (db);
	sprintf (buffer, schedule_query, cur_time, cur_time, cur_time, cur_time, daylight*3600);
	db_execute (db, buffer, playlist_template_query_cb, &data);
	if (data.l == NULL)
	{
		db_unlock (db);
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "no template found\n");
		return NULL;
	}
	assert (list_length (data.l) >= 1);
	t = data.l->data;
	list_free (data.l);
	t->events = get_playlist_events_from_template (db, t->id);

	if (t->repetition != 0.0) {
		
		/* Figure out th start and nd times of the current
		 * repeition of this template
		 *
		 */

		t->start_time = cur_time -
			(int) (cur_time-t->start_time-is_daylight(t->start_time)*3600+daylight*3600)%(int) t->repetition;
	}
	t->end_time += t->start_time;
	
	db_unlock (db);
  
	return t;
}



PlaylistTemplate *
get_playlist_template_by_id (Database *db, int id)
{
	PlaylistTemplate *t;
	char buffer[1024];
	QueryCallbackData data;
	assert (db != NULL);
	data.db = db;
	data.l = NULL;
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "get_playlist_template_by_id: id=%d\n", id);

	db_lock (db);
	sprintf (buffer, "select template_id, template_name, repeat_events, handle_overlap, artist_exclude, recording_exclude from playlist_template where template_id = %d", id);
	db_execute (db, buffer, playlist_template_query_cb, &data);
	if (data.l == NULL)
	{
		db_unlock (db);
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "no template found\n");
		return NULL;
	}
	assert (list_length (data.l) == 1);
	t = data.l->data;
	list_free (data.l);
	t->events = get_playlist_events_from_template (db, t->id);
	t->start_time = -1;
	t->end_time = -1;
	t->fallback_id = -1;
	t->end_prefade = 0.0;
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
		"create table prs_user ("
      "user_id integer primary key,"
      "user_name varchar (20),"
      "password varchar (20),"
      "email varchar (255),"
      "type varchar (20))";

	assert (db != NULL);
	db_lock (db);
	create_table (db,
		      "prs_user",
		      prs_users_create_query,
		      1);
	db_unlock (db);
}



static int
id_query_cb (void *udata, int cols, char **row, char **col_names)
{
	int *id_p = (int *) udata;
	assert (row != NULL);
	*id_p = atoi (row[0]);
	return 0;
}

void
add_recording (Recording *r, Database *db)
{
	int artist_id = -1;
	int category_id = -1;
	char buffer[2048];
	char *recording_name;
	char *recording_path;
	char *artist_name;
	char *category_name;
	char *recording_date;
	char *recording_insert_query =
		"insert into recording"
      "(recording_name,"
      "recording_path, artist_id,"
      "category_id, date,"
      "rate, channels,"
      "length, audio_in,"
       "audio_out) values ("
      "'%s', '%s', %d, %d, '%s',"
      "%d, %d, %lf, %lf, %lf)";
      
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
	db_execute (db, buffer, id_query_cb, &artist_id);
	if (artist_id == -1)
	{
		sprintf (buffer, "insert into artist (artist_name) values ('%s')", 
			 artist_name);
		db_execute (db, buffer, NULL, NULL);
		artist_id = sqlite_last_insert_rowid (db->db);
	}
	sprintf (buffer,
		 "select category_id from category where category_name = '%s'",
		 category_name);
	db_execute (db, buffer, id_query_cb, &category_id);
	if (category_id == -1)
	{
		sprintf (buffer, "insert into category (category_name) values ('%s')",
			 category_name);
		db_execute (db, buffer, NULL, NULL);
		category_id = sqlite_last_insert_rowid (db->db);
	}

	sprintf (buffer, recording_insert_query, recording_name, recording_path,
		 artist_id, category_id, recording_date, r->rate, r->channels,
		 r->length, r->audio_in, r->audio_out);
	db_execute (db, buffer, NULL, NULL);
	r->db = db;
	r->id = sqlite_last_insert_rowid (db->db);
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
	db_execute (r->db, buffer, NULL, NULL);
	db_unlock (r->db);
	r->db = NULL;
}



Recording *
find_recording_by_path (Database *db, const char *path)
{
	QueryCallbackData data;
	Recording *r;
	char buffer[1024];
	char *recording_path;
	char *select_query = 
      "select recording_id, recording_name, recording_path,"
      "artist_name, category_name, date,"
      "rate, channels,"
      "length, audio_in, audio_out "
      "from recording, artist, category "
      "where recording.artist_id = artist.artist_id and "
      "recording.category_id = category.category_id";

	assert (db != NULL);
	assert (path != NULL);
	data.db = db;
	data.l = NULL;
	db_lock (db);
	recording_path = process_for_sql (path);
	sprintf (buffer, "%s and recording.recording_path = '%s'", select_query,
		 recording_path);
	free (recording_path);
	db_execute (db, buffer, recording_query_cb, &data);
	if (data.l == NULL) {
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "recording with path %s not found\n", path);
		r = NULL;
	} else {
		assert (list_length (data.l) == 1);
		r = data.l->data;
		list_free (data.l);
	}
	db_unlock (db);
	return r;
}



list *
get_recordings (Database *db)
{
	char *err;
	QueryCallbackData data;
	char buffer[1024];
	int i = 0;

	if (!db)
		return NULL;
	data.db = db;
	data.l = NULL;
	db_lock (db);
	sprintf (buffer, "select * from recording");
	sqlite_exec (db->db, buffer, recording_query_cb, &data, &err);
	free (err);
	db_unlock (db);
	return data.l;
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



typedef struct
{
	int first;
	int *strlen_p;
	list **lp;
} PickerQueryData;

static int
picker_query_cb (void *udata, int cols, char **row, char **col_names)
{
	PickerQueryData *data = (PickerQueryData *) udata;

	if (data->first)
		data->first = 0;
	else
		*(data->strlen_p) += 2;

	*(data->lp) = string_list_append (*(data->lp), row[0]);
	*(data->strlen_p) += strlen (row[0]);
}

Recording *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 double cur_time)
{
	list *tmp;
	char buffer[2048];
	char category_part[1024];
	int n;
	Recording *r;
	char *select_query = 
		"select recording_id, recording_name, recording_path,"
      "artist_name, category_name, date,"
      "rate, channels,"
      "length, audio_in, audio_out "
      "from recording, artist, category "
      "where recording.artist_id = artist.artist_id and "
      "recording.category_id = category.category_id ";
	time_t ct;
	PickerQueryData pdata;
	QueryCallbackData rdata;
    
	assert (p != NULL);
	assert (p->db != NULL);

	rdata.db = p->db;
	rdata.l = NULL;

	debug_printf (DEBUG_FLAGS_DATABASE,
		      "recording_picker_select: cur_time=%f\n", cur_time);

	strcpy (buffer, select_query);
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
	if (category_list) {
		strcat (category_part, ")");
		strcat (buffer, category_part);
	}
	if (cur_time >= 0) {
		char exclude_part[1024];
		char temp[1024];

		/* Delete old items from exclude tables */

		sprintf (temp, "delete from %s where %s.time < %ld",
			 p->recording_exclude_table_name,
			 p->recording_exclude_table_name,
			 (long) (cur_time-p->recording_exclude));
		db_execute (p->db, temp, NULL, NULL);
		sprintf (temp, "delete from %s where %s.time < %ld",
			 p->artist_exclude_table_name,
			 p->artist_exclude_table_name,
			 (long) (cur_time-p->artist_exclude));
		db_execute (p->db, temp, NULL, NULL);
		sprintf (exclude_part, " and recording.recording_id not in (select recording_id from %s) and artist_name not in (select artist_name from %s)", p->recording_exclude_table_name, p->artist_exclude_table_name);
		strcat (buffer, exclude_part);
	}
	db_execute (p->db, buffer, &recording_query_cb, &rdata);
	if (rdata.l == NULL)
	{
		db_unlock (p->db);
		debug_printf (DEBUG_FLAGS_DATABASE, "no recording found\n");
		return NULL;
	}
	n = rand () % list_length (rdata.l);
	r = list_get_item (rdata.l, n);

	/* Add info to the exclude tables */

	if (cur_time >= 0)
	{
		char *artist_name = process_for_sql (r->artist);
		sprintf (buffer, "insert into %s values (%d, %lf)",
			 p->recording_exclude_table_name, r->id, cur_time);
		db_execute (p->db, buffer, NULL, NULL);
		sprintf (buffer, "insert into %s values ('%s', %lf)",
			 p->artist_exclude_table_name, artist_name, cur_time);
		db_execute (p->db, buffer, NULL, NULL);
		free (artist_name);
	}  

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
		"create table log ("
      "recording_id int,"
      "start_time int,"
      "length int)";
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
	db_execute (db, buffer, NULL, NULL);
	db_unlock (db);
}
