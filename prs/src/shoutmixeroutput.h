#ifndef _SHOUT_MIXER_OUTPUT_H
#define _SHOUT_MIXER_OUTPUT_H
#include <shout/shout.h>
#include "mixeroutput.h"



MixerOutput *
shout_mixer_output_new (const char *name,
			int rate,
			int channels,
			shout_conn_t *connection);
const shout_conn_t *
shout_mixer_output_get_connection (MixerOutput *o);
void
shout_mixer_output_set_connection (MixerOutput *o,
				   const shout_conn_t *connection);
#endif
