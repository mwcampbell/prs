#ifndef _DB_H
#define _DB_H
#include <mysql.h>
#include <pthread.h>
#include <libxml/parser.h>
#include "list.h"

typedef struct _Database Database;

struct _Database
{
  MYSQL *conn;
  pthread_mutex_t mutex;
};

Database *
db_new (void);
int
db_connect (Database *db, const char *host, const char *user,
	    const char *password, const char *name);
void
db_close (Database *db);
void
db_config (Database *db, xmlNodePtr cur);
void
db_thread_init (Database *db);
void
db_thread_end (Database *db);

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



typedef struct _PlaylistTemplate PlaylistTemplate;
struct _PlaylistTemplate {
  int id;
  char *name;
  double start_time;
  double end_time;
  int repeat_events;
  double artist_exclude;
  double recording_exclude;
  list *events;
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
  EVENT_TYPE_FADE
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
  double artist_exclude;
  double recording_exclude;
  Database *db;
};



RecordingPicker *
recording_picker_new (Database *db, double artist_exclude,
		      double recording_exclude);
void
recording_picker_destroy (RecordingPicker *p);
Recording *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 double cur_time);



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
