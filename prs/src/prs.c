#include <stdlib.h>
#include "mixer.h"
#include "mixerautomation.h"
#include "prs.h"
#include "scheduler.h"



PRS *
prs_new (void)
{
  PRS *prs = (PRS *) malloc (sizeof (PRS));

  if (prs == NULL)
    return NULL;

  prs->mixer = NULL;
  prs->automation = NULL;
  prs->scheduler = NULL;
  prs->telnet_interface = 0;
  prs->telnet_port = 0;
  prs->password = NULL;
  prs->mixer = mixer_new (2048);

  if (prs->mixer == NULL)
    {
      prs_destroy (prs);
      return NULL;
    }

  mixer_sync_time (prs->mixer);
  prs->automation = mixer_automation_new (prs->mixer);

  if (prs->automation == NULL)
    {
      prs_destroy (prs);
      return NULL;
    }

  prs->scheduler = scheduler_new (prs->automation,
				  mixer_get_time (prs->mixer));

  if (prs->scheduler == NULL)
    {
      prs_destroy (prs);
      return NULL;
    }

  return prs;
}



void
prs_start (PRS *prs)
{
  mixer_start (prs->mixer);
  scheduler_start (prs->scheduler, 10);

  if (prs->telnet_interface)
    mixer_automation_start (prs->automation);
}



void
prs_destroy (PRS *prs)
{
  if (prs == NULL)
    return;
  if (prs->scheduler != NULL)
    scheduler_destroy (prs->scheduler);
  if (prs->automation != NULL)
    mixer_automation_destroy (prs->automation);
  if (prs->mixer != NULL)
    mixer_destroy (prs->mixer);
  if (prs->password != NULL)
    free (prs->password);
  free (prs);
}
