#ifndef _MIXER_EVENT_H
#define _MIXER_EVENT_H



typedef enum {
  MIXER_EVENT_TYPE_ADD_CHANNEL
} MIXER_EVENT_TYPE;



typedef struct _MixerEvent MixerEvent;



struct _MixerEvent {
  MIXER_EVENT_TYPE type;
  double time;
  char *detail1;
  char *detail2;
};



#endif
