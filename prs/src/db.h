#ifndef _DB_H
#define _DB_H
#include "list.h"

/*
 *
 * Recording information
 *
 */



typedef struct _RecordingInfo RecordingInfo;

struct _RecordingInfo {
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



void
create_recording_table (list *read_only_users,
			list *total_access_users);
void
recording_info_free (RecordingInfo *i);



/*
 *
 * Lists of recording information
 *
 */



void
recording_info_list_free (list *l);



/*
 *
 * Playlist Template Information
 *
 */



typedef struct _PlaylistTemplateInfo PlaylistTemplateInfo;
struct _PlaylistTemplateInfo {
  int id;
  char *name;
  double start_time;
  double end_time;
  int repeat_events;
  double artist_exclude;
  double recording_exclude;
};



void
create_playlist_template_table (list *read_only_users,
				list *total_access_users);
void
playlist_template_info_free (PlaylistTemplateInfo *i);

PlaylistTemplateInfo *
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
create_playlist_event_table (list *read_only_users,
			     list *total_access_users);
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
get_playlist_events_from_template (PlaylistTemplateInfo *t);



/*
 *
 * PRS users
 *
 */



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



int
create_table (const char *table_name,
	      const char *create_query,
	      list *read_only_access_list,
	      list *total_access_list,
	      int erase_existing);



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
RecordingInfo *
recording_picker_select (RecordingPicker *p,
			 list *category_list,
			 double cur_time);



#endif
