#ifndef _DB_H
#define _DB_H
#include "list.h"

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
};



int
check_recording_tables (void);
void
create_recording_tables (list *read_only_users,
			list *total_access_users);
void
recording_free (Recording *r);
void
add_recording (Recording *r);
void
delete_recording (Recording *r);
Recording *
find_recording_by_path (const char *path);



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
};



int
check_playlist_tables (void);
void
create_playlist_tables (list *read_only_users,
				list *total_access_users);
void
playlist_template_free (PlaylistTemplate *t);

PlaylistTemplate *
get_playlist_template (double time);



/*
 *
 * laylist events
 *
 */



typedef struct _PlaylistEvent PlaylistEvent;

typedef enum {
  EVENT_TYPE_SIMPLE_RANDOM,
  EVENT_TYPE_RANDOM
} playlist_event_type;

struct _PlaylistEvent {
  int template_id;
  int number;
  playlist_event_type type;
  int relative_to_end;
  double offset;
  char *detail1;
  char *detail2;
  char *detail3;
  char *detail4;
  char *detail5;
};



void
playlist_event_free (PlaylistEvent *e);



/*
 *
 * Playlist event list
 *
 */

void
playlist_event_list_free (list *l);
list *
get_playlist_events_from_template (PlaylistTemplate *t);



/*
 *
 * PRS users
 *
 */



int
check_user_table (void);
void
create_user_table (list *read_only_users,
		   list *total_access_users);



/*
 *
 * Database connection functions
 *
 */



int
connect_to_database (const char *dbname);
void
disconnect_from_database (void);



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
};



RecordingPicker *
recording_picker_new (double artist_exclude,
		      double recording_exclude);
void
recording_picker_free (RecordingPicker *p);
Recording *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 double cur_time);



#endif
