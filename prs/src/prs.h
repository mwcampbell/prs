#ifndef _PRS_H
#define _PRS_H
#include "mixer.h"
#include "db.h"
#include "mixerautomation.h"
#include "scheduler.h"



typedef struct
{
  mixer *mixer;
  Database *db;
  MixerAutomation *automation;
  scheduler *scheduler;
  int telnet_interface;
  int telnet_port;
  char *password;
}
PRS;



PRS *
prs_new (void);
void
prs_start (PRS *prs);
void
prs_destroy (PRS *prs);

#endif
