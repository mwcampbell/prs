#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <vorbis/vorbisfile.h>
#include "mixerchannel.h"



typedef struct {
	OggVorbis_File *vf;
	int section;
	FILE *fp;
} vorbis_file_info;



static void
vorbis_mixer_channel_get_data (MixerChannel *ch)
{
	vorbis_file_info *i;
	int remainder;
	short *tmp;
	int rv;
  
	if (!ch)
		return;
	tmp = ch->buffer;
	remainder = ch->buffer_size;
	i = (vorbis_file_info *) ch->data;
	while (remainder > 0) {
		rv = ov_read ((i->vf), (char *)tmp, remainder*sizeof(short), 0, sizeof(short), 1, &(i->section));
		if (rv <= 0)
			break;
		remainder -= rv/sizeof(short);
		tmp += rv/sizeof(short);
	}
	if (remainder) {

		/* We've reached the end of the data */

		ch->data_end_reached = 1;
		ch->buffer_length = ch->buffer_size-remainder;
	}
	else
		ch->buffer_length = ch->buffer_size;
	return;
}



static void
vorbis_mixer_channel_free_data (MixerChannel *ch)
{
	vorbis_file_info *i;

	if (!ch)
		return;
	i = (vorbis_file_info *) ch->data;
	if (!i)
		return;
	ov_clear (i->vf);
	free (i->vf);
	free (i);
}



MixerChannel *
vorbis_mixer_channel_new (const char *name,
			  const char *location,
			  const int mixer_latency)
{
	MixerChannel *ch;
	vorbis_file_info *i;
	vorbis_info *vi;
  
	i = malloc (sizeof (vorbis_file_info));

	/* Allocate the OggVorbis_file structure */

	i->vf = malloc (sizeof(OggVorbis_File));
  
	/* Open the file */

	i->fp = fopen (location, "rb");
	if (!(i->fp)) {
		free (i);
		return NULL;
	}
  
	ov_open ((i->fp), (i->vf), NULL, 0);

	/* Get sample rate information from file */

	vi = ov_info (i->vf, -1);
	ch = mixer_channel_new (vi->rate,
				vi->channels,
				mixer_latency);

	ch->data = (void *) i;
	ch->name = strdup (name);
	ch->location = strdup (location);
  
	/* Set overrideable methods */

	ch->get_data = vorbis_mixer_channel_get_data;
	ch->free_data = vorbis_mixer_channel_free_data;

	return ch;
}
