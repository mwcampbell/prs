#include <assert.h>
#include <stdlib.h>
#include "db.h"
#include "debug.h"
#include "mixer.h"
#include "mixerautomation.h"
#include "prs.h"
#include "scheduler.h"



PRS *
prs_new (void)
{
	PRS *prs = NULL;
	debug_printf (DEBUG_FLAGS_GENERAL, "prs_new called\n");
	prs = (PRS *) malloc (sizeof (PRS));
	assert (prs != NULL);
	prs->mixer = NULL;
	prs->automation = NULL;
	prs->scheduler = NULL;
	prs->telnet_interface = 0;
	prs->telnet_port = 0;
	prs->password = NULL;
	prs->mixer = mixer_new (2048);
	prs->db = db_new ();
	mixer_sync_time (prs->mixer);
	prs->automation = mixer_automation_new (prs->mixer, prs->db);
	prs->scheduler = scheduler_new (prs->automation, prs->db,
					mixer_get_time (prs->mixer));
	return prs;
}



void
prs_start (PRS *prs)
{
	assert (prs != NULL);
	debug_printf (DEBUG_FLAGS_GENERAL, "prs_start_called\n");
	mixer_start (prs->mixer);
	scheduler_start (prs->scheduler, 10);
	mixer_automation_start (prs->automation);
}



void
prs_destroy (PRS *prs)
{
	assert (prs != NULL);
	debug_printf (DEBUG_FLAGS_GENERAL, "prs_destroy called\n");
	if (prs->scheduler != NULL)
		scheduler_destroy (prs->scheduler);
	if (prs->automation != NULL)
		mixer_automation_destroy (prs->automation);
	if (prs->db != NULL)
		db_close (prs->db);
	if (prs->mixer != NULL)
		mixer_destroy (prs->mixer);
	if (prs->password != NULL)
		free (prs->password);
	free (prs);
}
