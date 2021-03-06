/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _DB_H
#define _DB_H
#include <sqlite3.h>
#include <pthread.h>
#include <libxml/parser.h>
#include "list.h"

typedef struct _Database Database;

struct _Database
{
	sqlite3 *db;
	pthread_mutex_t mutex;
};

Database *
db_new (void);
int
db_open (Database *db, const char *filename);
void
db_close (Database *db);
void
db_config (Database *db, xmlNodePtr cur);
void
db_begin_transaction (Database *db);
void
db_commit (Database *db);

/*
 *
 * Recording information
 *
 */



typedef struct _Recording Recording;

struct _Recording {
	int id;
	char *name;
	char *path;
	char *artist;
	char *category;
	char *date;
	int rate;
	int channels;
	double length;
	double audio_in;
	double audio_out;
	Database *db;
};



int
check_recording_tables (Database *db);
void
create_recording_tables (Database *db);
void
recording_free (Recording *r);
void
add_recording (Recording *r, Database *db);
void
delete_recording (Recording *r);
Recording *
find_recording_by_path (Database *db, const char *path);
list *
get_recordings (Database *db);



/*
 *
 * Lists of recordings
 *
 */



void
recording_list_free (list *l);



/*
 *
 * Playlist Template Information
 *
 */



typedef enum {
	TEMPLATE_TYPE_STANDARD,
	TEMPLATE_TYPE_FALLBACK
} playlist_template_type;


typedef enum {
	HANDLE_OVERLAP_INVALID,
	HANDLE_OVERLAP_DISCARD,
	HANDLE_OVERLAP_FALLBACK,
	HANDLE_OVERLAP_FADE,
	HANDLE_OVERLAP_IGNORE
} handle_overlap_type;


typedef struct _PlaylistTemplate PlaylistTemplate;
struct _PlaylistTemplate {
	int id;
	playlist_template_type type;
	char *name;
	double start_time;
	double end_time;
	int repeat_events;
	handle_overlap_type handle_overlap;
	int artist_exclude;
	int recording_exclude;
	list *events;
	int fallback_id;
	double end_prefade;
	Database *db;
};



int
check_playlist_tables (Database *db);
void
create_playlist_tables (Database *db);
void
playlist_template_destroy (PlaylistTemplate *t);
PlaylistTemplate *
get_playlist_template (Database *db, double cur_time);
PlaylistTemplate *
get_playlist_template_by_id (Database *db, int id);



/*
 *
 * Playlist events
 *
 */



typedef struct _PlaylistEvent PlaylistEvent;

typedef enum {
	EVENT_TYPE_SIMPLE_RANDOM,
	EVENT_TYPE_RANDOM,
	EVENT_TYPE_PATH,
	EVENT_TYPE_FADE,
	EVENT_TYPE_URL
} playlist_event_type;

struct _PlaylistEvent {
	int template_id;
	int number;
	playlist_event_type type;
	char *channel_name;
	double level;
	int anchor_event_number;
	int anchor_position;
	double offset;
	char *detail1;
	char *detail2;
	char *detail3;
	char *detail4;
	char *detail5;
	double start_time;
	double end_time;
};



void
playlist_event_free (PlaylistEvent *e);
PlaylistEvent *
playlist_template_get_event (PlaylistTemplate *t,
			     int event_number);



/*
 *
 * PRS users
 *
 */



int
check_user_table (Database *db);
void
create_user_table (Database *db);



/*
 *
 * Creating tables
 *
 */






/*
 *
 * Recording picker
 *
 */



typedef struct _RecordingPicker RecordingPicker;
struct _RecordingPicker {
	char *artist_exclude_table_name;
	char *recording_exclude_table_name;
	int artist_exclude;
	int recording_exclude;
	Database *db;
};



RecordingPicker *
recording_picker_new (Database *db, int artist_exclude,
		      int recording_exclude);
void
recording_picker_destroy (RecordingPicker *p);
Recording *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 int avoid_repeating);



/*
 *
 * Log table
 *
 */

int
check_log_table (Database *db);
void
create_log_table (Database *db);
void
add_log_entry (Database *db, int recording_id, int start_time, int length);



#endif
