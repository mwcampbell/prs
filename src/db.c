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
#include <sqlite3.h>
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
	db->db = NULL;
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
db_open (Database *db, const char *filename)
{
	int rc;
	assert (db != NULL);
	assert (filename != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "db_open: filename=%s\n", filename);
	db_lock (db);
	rc = sqlite3_open (filename, &(db->db));
	db_unlock (db);

	if (rc == SQLITE_OK)
		return 0;
	else
	{
		fprintf (stderr, "Unable to open database: %s\n",
			 sqlite3_errmsg (db->db));
		db_close (db);
		exit (EXIT_FAILURE);
	}
}

static void
db_execute (Database *db, const char *stmt)
{
	char *errmsg;
	int rc;
	assert (db != NULL);
	assert (stmt != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "db: executing: %s\n", stmt);
	rc = sqlite3_exec (db->db, stmt, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK) {
		fprintf (stderr, "SQLite error: %s\n", errmsg);
		sqlite3_free (errmsg);
		exit (EXIT_FAILURE);
	}
}

static void
db_query (Database *db, const char *query, char ***presult, int *pnrows,
	  int *pncolumns)
{
	char *errmsg;
	int rc;
	assert (db != NULL);
	assert (query != NULL);
	assert (presult != NULL);
	assert (pnrows != NULL);
	assert (pncolumns != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "db: executing: %s\n", query);
	rc = sqlite3_get_table (db->db, query, presult, pnrows,
				pncolumns, &errmsg);
	if (rc != SQLITE_OK) {
		fprintf (stderr, "SQLite error: %s\n", errmsg);
		sqlite3_free (errmsg);
		exit (EXIT_FAILURE);
	}
	assert (presult != NULL);
}

void
db_close (Database *db)
{
	assert (db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE, "closing database\n");
	db_lock (db);
	if (db->db)
		sqlite3_close (db->db);
	db_unlock (db);
	free (db);
}

void
db_config (Database *db, xmlNodePtr cur)
{
	xmlChar *filename = NULL;
	filename = xmlGetProp (cur, (xmlChar*)"filename");
	db_open (db, (char*)filename);
	if (filename)
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
	int rc;
	char query[2048];
  
	assert (db != NULL);
	assert (table_name != NULL);
	assert (create_query != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "create_table: name=%s, erase_existing=%d\n",
		      table_name, erase_existing);

	/* Does table already exist */

	sprintf (query, "select count(*) from %s", table_name);
	rc = sqlite3_exec (db->db, query, NULL, NULL, NULL);
	if (rc == SQLITE_OK) {
		debug_printf (DEBUG_FLAGS_DATABASE, "table exists\n");
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
	int rc;
	char query[1024];
	int rv;

	assert (db != NULL);
	assert (table_name != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "checking whether table %s exists\n", table_name);
	sprintf (query, "select count(*) from %s", table_name);

	rc = sqlite3_exec (db->db, query, NULL, NULL, NULL);
	if (rc == SQLITE_OK) {
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "table %s exists\n", table_name);
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
get_recording_from_result (Database *db, char **result, int rowindex,
			   int ncolumns)
{
	Recording *r = NULL;
	assert (db != NULL);
	assert (result != NULL);
	r = (Recording *) malloc (sizeof(Recording));
	assert (r != NULL);
	r->db = db;

	/* Get recording information from database result */

	r->id = atoi (result[rowindex * ncolumns + 0]);
	r->name = strdup (result[rowindex * ncolumns + 1]);
	r->path = strdup (result[rowindex * ncolumns + 2]);
	r->artist = strdup (result[rowindex * ncolumns + 3]);
	r->category = strdup (result[rowindex * ncolumns + 4]);
	r->date = strdup (result[rowindex * ncolumns + 5]);
	r->rate = atoi (result[rowindex * ncolumns + 6]);
	r->channels = atoi (result[rowindex * ncolumns + 7]);
	r->length = atof (result[rowindex * ncolumns + 8]);
	r->audio_in = atof (result[rowindex * ncolumns + 9]);
	r->audio_out = atof (result[rowindex * ncolumns + 10]);
	return r;
}



static PlaylistEvent *
get_playlist_event_from_result (char **result, int rowindex, int ncolumns)
{
	PlaylistEvent *e = NULL;
	char *type;

	assert (result != NULL);
	e = (PlaylistEvent *) malloc (sizeof(PlaylistEvent));
	assert (e != NULL);
	e->template_id = atoi (result[rowindex * ncolumns + 0]);
	e->number = atoi (result[rowindex * ncolumns + 1]);
	type = result[rowindex * ncolumns + 3];
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

	e->channel_name = strdup (result[rowindex * ncolumns + 4]);
	e->level = atof (result[rowindex * ncolumns + 5]);
	e->anchor_event_number = atoi (result[rowindex * ncolumns + 6]);
	e->anchor_position = atoi (result[rowindex * ncolumns + 7]);
	e->offset = atof (result[rowindex * ncolumns + 8]);
	e->detail1 = strdup (result[rowindex * ncolumns + 9]);
	e->detail2 = strdup (result[rowindex * ncolumns + 10]);
	e->detail3 = strdup (result[rowindex * ncolumns + 11]);
	e->detail4 = strdup (result[rowindex * ncolumns + 12]);
	e->detail5 = strdup (result[rowindex * ncolumns + 13]);
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
	prs_list_free (l);
}



static list *
get_playlist_events_from_template (Database *db,
				   int template_id)
{
	char **result = NULL;
	int nrows;
	int ncolumns;
	int rowindex;
	char buffer[1024];
	list *events = NULL;
  
	assert (db != NULL);
	assert (template_id >= 0);
	sprintf (buffer, "select * from playlist_event where template_id = %d order by event_number desc", template_id);
	db_query (db, buffer, &result, &nrows, &ncolumns);
	for (rowindex = 1; rowindex <= nrows; rowindex++) {
		PlaylistEvent *e = get_playlist_event_from_result (result, rowindex, ncolumns);
		events = prs_list_prepend (events, (void *) e);
	}

	sqlite3_free_table (result);
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
      "artist_id integer primary key, "
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
      "artist_id integer,"
      "category_id integer,"
      "date varchar (20),"
      "rate integer,"
      "channels integer,"
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
	prs_list_free (l);
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
      "repeat_events integer,"
      "handle_overlap integer,"
      "artist_exclude integer,"
      "recording_exclude integer)";
	char *create_schedule_query =
    "create table schedule ("
    "time_slot_id integer primary key,"
    "start_time double,"
    "length double,"
    "repetition double,"
    "daylight integer,"
    "template_id integer,"
    "fallback_id integer,"
    "end_prefade double)";

	char *create_playlist_event_query =
      "create table playlist_event ("
      "template_id integer,"
      "event_number integer,"
      "event_name varchar (100),"
      "event_type varchar (20),"
      "event_channel_name varchar (100),"
      "event_level double,"
      "event_anchor_event_number integer,"
      "event_anchor_position integer,"
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



static PlaylistTemplate *
get_playlist_template_from_result (Database *db,
				   char **result, int rowindex, int ncolumns)
{
	PlaylistTemplate *t = NULL;

	assert (db != NULL);
	assert (result != NULL);
	t = (PlaylistTemplate *) malloc (sizeof(PlaylistTemplate));
	assert (t != NULL);
	t->id = atoi (result[rowindex * ncolumns + 0]);
	t->name = strdup (result[rowindex * ncolumns + 1]);
	t->repeat_events = atoi (result[rowindex * ncolumns + 2]);
	t->handle_overlap = atoi (result[rowindex * ncolumns + 3]);
	t->artist_exclude = atoi (result[rowindex * ncolumns + 4]);
	t->recording_exclude = atoi (result[rowindex * ncolumns + 5]);
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
	char **result = NULL;
	int nrows;
	int ncolumns;
	int rowindex;
	char buffer[1024];
	int daylight;
	double repetition;
	char *schedule_query =
    "select schedule.template_id, template_name, repeat_events, handle_overlap, "
    "artist_exclude, recording_exclude , start_time, "
    "length, repetition, fallback_id, "
    "end_prefade from playlist_template, schedule "
    "where playlist_template.template_id = schedule.template_id and "
    "((repetition = 0 and start_time <= %lf and start_time+length > %lf) or "
    "(repetition != 0 and start_time <= %lf and ((%lf-start_time-(daylight*3600)+%d) %% repetition) < length)) "
    "order by time_slot_id desc";

	daylight = is_daylight (cur_time);
	assert (db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "get_playlist_template: cur_time=%f\n", cur_time);
	db_lock (db);
	sprintf (buffer, schedule_query, cur_time, cur_time, cur_time, cur_time, daylight*3600);
	db_query (db, buffer, &result, &nrows, &ncolumns);
	if (nrows < 1) {
		sqlite3_free_table (result);
		db_unlock (db);
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "no template found\n");
		return NULL;
	}
	rowindex = 1;
	t = get_playlist_template_from_result (db, result, rowindex, ncolumns);
	t->start_time = atof (result[rowindex * ncolumns + 6]);
	t->end_time = atof (result[rowindex * ncolumns + 7]);
	repetition = atof (result[rowindex * ncolumns + 8]);

	if (repetition != 0) {
		
		/* Figure out th start and nd times of the current
		 * repeition of this template
		 *
		 */

		t->start_time = cur_time -
			(int) (cur_time-t->start_time-is_daylight(t->start_time)*3600+daylight*3600)%(int) repetition;
	}
	t->end_time += t->start_time;
	
	t->fallback_id = atoi (result[rowindex * ncolumns + 9]);
	t->end_prefade = atof(result[rowindex * ncolumns + 10]);;
	sqlite3_free_table (result);
	db_unlock (db);
  
	return t;
}



PlaylistTemplate *
get_playlist_template_by_id (Database *db, int id)
{
	PlaylistTemplate *t;
	char **result = NULL;
	int nrows;
	int ncolumns;
	int rowindex;
	char buffer[1024];

	assert (db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "get_playlist_template_by_id: id=%d\n", id);

	db_lock (db);
	sprintf (buffer, "select template_id, template_name, repeat_events, handle_overlap, artist_exclude, recording_exclude from playlist_template where template_id = %d", id);
	db_query (db, buffer, &result, &nrows, &ncolumns);
	if (nrows != 1) {
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "no template found\n");
		sqlite3_free_table (result);
		db_unlock (db);
		return NULL;
	}
	rowindex = 1;
	t = get_playlist_template_from_result (db, result, rowindex, ncolumns);
	t->start_time = -1;
	t->end_time = -1;
	t->fallback_id = -1;
	t->end_prefade = 0.0;
	sqlite3_free_table (result);
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
	e = (PlaylistEvent *) prs_list_get_item (t->events, event_number-1);
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



void
add_recording (Recording *r, Database *db)
{
	char **result = NULL;
	int nrows;
	int ncolumns;
	int rowindex;
	int artist_id;
	int category_id;
	char buffer[2048];
	char *recording_name;
	char *recording_path;
	char *artist_name;
	char *category_name;
	char *recording_date;
	char *recording_insert_query =
      "insert into recording "
      "(recording_name, "
      "recording_path, artist_id, "
      "category_id, date, "
      "rate, channels, "
      "length, audio_in, "
       "audio_out) values ("
      "'%s', '%s', %d, %d, '%s', "
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
	db_query (db, buffer, &result, &nrows, &ncolumns);
	if (nrows == 1) {
		rowindex = 1;
		artist_id = atoi (result[rowindex * ncolumns + 0]);
		sqlite3_free_table (result);
	}
	else {
		sqlite3_free_table (result);
		sprintf (buffer, "insert into artist (artist_name) values ('%s')", 
			 artist_name);
		db_execute (db, buffer);
		artist_id = sqlite3_last_insert_rowid (db->db);
	}
	sprintf (buffer,
		 "select category_id from category where category_name = '%s'",
		 category_name);
	db_query (db, buffer, &result, &nrows, &ncolumns);
	if (nrows == 1) {
		rowindex = 1;
		category_id = atoi (result[rowindex * ncolumns + 0]);
		sqlite3_free_table (result);
	}
	else {
		sqlite3_free_table (result);
		sprintf (buffer, "insert into category (category_name) values ('%s')",
			 category_name);
		db_execute (db, buffer);
		category_id = sqlite3_last_insert_rowid (db->db);
	}

	sprintf (buffer, recording_insert_query, recording_name, recording_path,
		 artist_id, category_id, recording_date, r->rate, r->channels,
		 r->length, r->audio_in, r->audio_out);
	db_execute (db, buffer);
	r->db = db;
	r->id = sqlite3_last_insert_rowid (db->db);
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
	char **result = NULL;
	int nrows;
	int ncolumns;
	int rowindex;
	Recording *r;
	char buffer[1024];
	char *recording_path;
	char *select_query = 
      "select recording_id, recording_name, recording_path, "
      "artist_name, category_name, date, "
      "rate, channels, "
      "length, audio_in, audio_out "
      "from recording, artist, category "
      "where recording.artist_id = artist.artist_id and "
      "recording.category_id = category.category_id";

	assert (db != NULL);
	assert (path != NULL);
	db_lock (db);
	recording_path = process_for_sql (path);
	sprintf (buffer, "%s and recording.recording_path = '%s'", select_query,
		 recording_path);
	free (recording_path);
	db_query (db, buffer, &result, &nrows, &ncolumns);
	if (nrows != 1) {
		debug_printf (DEBUG_FLAGS_DATABASE,
			      "recording with path %s not found\n", path);
		r = NULL;
	} else {
		rowindex = 1;
		r = get_recording_from_result (db, result, rowindex, ncolumns);
	}
	sqlite3_free_table (result);
	db_unlock (db);
	return r;
}



list *
get_recordings (Database *db)
{
	char **result = NULL;
	int nrows;
	int ncolumns;
	int rowindex;
	Recording *r;
	char buffer[1024];
	list *rv = NULL;
	

	if (!db)
		return NULL;
	db_lock (db);
	sprintf (buffer, "select * from recording");
	if (sqlite3_get_table (db->db, buffer, &result, &nrows, &ncolumns, NULL) != SQLITE_OK) {
		db_unlock (db);
		return NULL;
	}
	for (rowindex = 1; rowindex <= nrows; rowindex++) {
		r = get_recording_from_result (db, result, rowindex, ncolumns);
		rv = prs_list_prepend (rv, r);
	}
	sqlite3_free_table (result);
	db_unlock (db);
	return rv;
}


RecordingPicker *
recording_picker_new (Database *db, int artist_exclude,
		      int recording_exclude)
{
	static int randomized = 0;
	char buffer[1024];
	int i;
	RecordingPicker *p = NULL;

	assert (db != NULL);
	p = (RecordingPicker *) malloc (sizeof (RecordingPicker));
	assert (p != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "recording_picker_new: artist_exclude=%g, "
		      "recording_exclude=%g\n", artist_exclude,
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

	sprintf (buffer, "create table %s (recording_exclude_id integer primary key, recording_id integer)",
		 p->recording_exclude_table_name);
	create_table (db,
		      p->recording_exclude_table_name,
		      buffer,
		      0);
	sprintf (buffer, "create table %s (artist_exclude_id integer primary key, artist_name varchar(200))",
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

	free (p);
}



Recording *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 int avoid_repeating)
{
	list *tmp;
	char **result = NULL;
	int nrows;
	int ncolumns;
	int rowindex;
	char *buffer;
	char category_part[1024];
	int n;
	Recording *r;
	char *select_query = 
      "select recording_id, recording_name, recording_path, "
      "artist_name, category_name, date, "
      "rate, channels, "
      "length, audio_in, audio_out "
      "from recording, artist, category "
      "where recording.artist_id = artist.artist_id and "
      "recording.category_id = category.category_id";

	assert (p != NULL);
	assert (p->db != NULL);
	debug_printf (DEBUG_FLAGS_DATABASE,
		      "recording_picker_select: avoid_repeating=%d\n", avoid_repeating);

	db_lock (p->db);
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
	if (avoid_repeating) {
		list *artists = NULL, *recordings = NULL;
		int artists_strlen = 0, recordings_strlen = 0, first;
		char temp[1024];

		sprintf (temp, "select artist_name from %s order by artist_exclude_id desc limit %d",
			 p->artist_exclude_table_name,
			 p->artist_exclude);
		db_query (p->db, temp, &result, &nrows, &ncolumns);
		first = 1;

		for (rowindex = 1; rowindex <= nrows; rowindex++) {
			if (first)
				first = 0;
			else
				artists_strlen += 2;

			artists = string_list_append (artists, result[rowindex * ncolumns + 0]);
			artists_strlen += strlen (result[rowindex * ncolumns + 0]);
		}

		sqlite3_free_table (result);
		sprintf (temp, "select recording_id from %s order by recording_exclude_id desc limit %d",
			 p->recording_exclude_table_name,
			 p->recording_exclude);
		db_query (p->db, temp, &result, &nrows, &ncolumns);
		first = 1;

		for (rowindex = 1; rowindex <= nrows; rowindex++) {
			if (first)
				first = 0;
			else
				recordings_strlen += 2;

			recordings = string_list_append (recordings, result[rowindex * ncolumns + 0]);
			recordings_strlen += strlen (result[rowindex * ncolumns + 0]);
		}

		sqlite3_free_table (result);
		buffer = malloc (artists_strlen + recordings_strlen + 2048);
		assert (buffer != NULL);
		strcpy (buffer, select_query);
		if (category_list)
			strcat (buffer, category_part);
		first = 1;

		for (tmp = recordings; tmp; tmp = tmp->next)
		{
			if (first)
			{
				strcat (buffer, " and recording.recording_id not in (");
				first = 0;
			}
			else
				strcat (buffer, ", ");

			strcat (buffer, tmp->data);
		}

		if (!first)
			strcat (buffer, ")");
		first = 1;

		for (tmp = artists; tmp; tmp = tmp->next)
		{
			char *tmp_str = process_for_sql (tmp->data);

			if (first)
			{
				strcat (buffer, " and artist_name not in (");
				first = 0;
			}
			else
				strcat (buffer, ", ");

			strcat (buffer, "'");
			strcat (buffer, tmp_str);
			strcat (buffer, "'");
			free (tmp_str);
		}

		if (!first)
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
	db_query (p->db, buffer, &result, &nrows, &ncolumns);
	if (nrows <= 0)
	{
		sqlite3_free_table (result);
		free (buffer);
		db_unlock (p->db);
		debug_printf (DEBUG_FLAGS_DATABASE, "no recording found\n");
		return NULL;
	}

	n = rand () % nrows;
	rowindex = n + 1;

	r = get_recording_from_result (p->db, result, rowindex, ncolumns);
	sqlite3_free_table (result);

	/* Add info to the exclude tables */

	if (avoid_repeating)
	{
		char *artist_name = process_for_sql (r->artist);
		sprintf (buffer, "insert into %s (recording_id) values (%d)",
			 p->recording_exclude_table_name, r->id);
		db_execute (p->db, buffer);
		sprintf (buffer, "insert into %s (artist_name) values ('%s')",
			 p->artist_exclude_table_name, artist_name);
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
      "create table log ("
      "recording_id integer,"
      "start_time integer,"
      "length integer)";

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
