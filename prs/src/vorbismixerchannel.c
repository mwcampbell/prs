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



static int
vorbis_mixer_channel_get_data (MixerChannel *ch,
		      short *buffer, int size)
{
  vorbis_file_info *i;
  int remainder = size;
  short *tmp = buffer;
  int rv;
  
  if (!ch)
    return 0;
  i = (vorbis_file_info *) ch->data;
  while (remainder > 0)
    {
      rv = ov_read ((i->vf), (char *)tmp, remainder*sizeof(short), 0, sizeof(short), 1, &(i->section));
      if (rv <= 0)
	break;
      remainder -= rv/sizeof(short);
      tmp += rv/sizeof(short);
  }
  if (remainder)
    {
      /* We've reached the end of the data */

      ch->data_end_reached = 1;
      return size-remainder;
    }
  else
    return size;
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
		   const char *location)
{
  MixerChannel *ch;
  vorbis_file_info *i;
  vorbis_info *vi;
  
  ch = malloc (sizeof(MixerChannel));
  i = malloc (sizeof (vorbis_file_info));

  /* Allocate the OggVorbis_file structure */

  i->vf = malloc (sizeof(OggVorbis_File));
  
  /* Open the file */

  i->fp = fopen (location, "rb");
  if (!(i->fp))
    {
      free (i);
      free (ch);
      return NULL;
    }
  
  ov_open ((i->fp), (i->vf), NULL, 0);
  ch->data = (void *) i;
  ch->name = strdup (name);
  ch->location = strdup (location);
  ch->enabled = 1;

  /* Set overrideable methods */

  ch->get_data = vorbis_mixer_channel_get_data;
  ch->free_data = vorbis_mixer_channel_free_data;

  /* Get sample information from file */

  vi = ov_info (i->vf, -1);
  ch->rate = vi->rate;
  ch->channels = vi->channels;

  /* Set level and fading parameters */

  ch->fade = 0.0;
  ch->level = 1.0;
  ch->fade_destination = 1.0;

  /* Set the end of data flag to 0 */

  ch->data_end_reached = 0;

  /* Default is patched to no outputs */

  ch->outputs = NULL;

  return ch;
}
