#ifndef _MIXER_EVENT_H
#define _MIXER_EVENT_H



typedef enum {
  MIXER_EVENT_TYPE_ADD_CHANNEL,
  MIXER_EVENT_TYPE_FADE_CHANNEL,
  MIXER_EVENT_TYPE_FADE_ALL,
  MIXER_EVENT_TYPE_DELETE_ALL  
} MIXER_EVENT_TYPE;



typedef struct _MixerEvent MixerEvent;



struct _MixerEvent {
  MIXER_EVENT_TYPE type;
  double start_time;
  double end_time;
  char *channel_name;
  double level;
  char *detail1;
  char *detail2;
};



void
mixer_event_free (MixerEvent *e);



#endif
