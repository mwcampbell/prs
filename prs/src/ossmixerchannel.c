#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include "mixerchannel.h"



typedef struct {
  int fd;
} oss_info;



static int
oss_mixer_channel_get_data (MixerChannel *ch,
		      short *buffer, int size)
{
  oss_info *i;
  int remainder = size;
  short *tmp = buffer;
  int rv;
  
  if (!ch)
    return 0;
  i = (oss_info *) ch->data;
  while (remainder > 0)
    {
      rv = read (i->fd, tmp, remainder*sizeof(short));
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
oss_mixer_channel_free_data (MixerChannel *ch)
{
  oss_info *i;
  int tmp;
  
  if (!ch)
    return;
  i = (oss_info *) ch->data;
  if (!i)
    return;
  if (!global_data_get_soundcard_duplex ())
    {
      close (i->fd);
      global_data_set_soundcard_fd (-1);
    }
  free (i);
}



MixerChannel *
oss_mixer_channel_new (const char *name,
		       int rate,
		       int channels)
{
  MixerChannel *ch;
  oss_info *i;
  int tmp;
  
  i = malloc (sizeof (oss_info));
  if (!i)
    return NULL;

  /* Open the sound device */

  i->fd = global_data_get_soundcard_fd ();
  if (i->fd < 0)
    {
      i->fd = open ("/dev/dsp", O_RDWR);
      if (i->fd < 0)
	{
	  global_data_set_soundcard_duplex (0);
	  i->fd = open ("/dev/dsp", O_RDONLY);
	}
      else
	global_data_set_soundcard_duplex (1);
      global_data_set_soundcard_fd (i->fd);

      /* Setup sound card */

      tmp = 0x00040009;
      if (ioctl (i->fd, SNDCTL_DSP_SETFRAGMENT, &tmp) < 0)
	{
	  close (i->fd);
	  free (i);
	  return NULL;
	}
      tmp = AFMT_S16_LE;
      if (ioctl (i->fd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0)
	{
	  close (i->fd);
	  free (i);
	  return NULL;
	}
      if (channels == 1)
	tmp = 0;
      else
	tmp = 1;
      if (ioctl (i->fd, SNDCTL_DSP_STEREO, &tmp) < 0)
	{
	  close (i->fd);
	  free (i);
	  return NULL;
	}
      tmp = rate;
      if (ioctl (i->fd, SNDCTL_DSP_SPEED, &tmp) < 0)
	{
	  close (i->fd);
	  free (i);
	  return NULL;
	}
  
    }
  if (i->fd < 0)
    {
      free (i);
      return NULL;
      }
  ch = (MixerChannel *) malloc (sizeof (MixerChannel));
  if (!ch)
    {
      close (i->fd);
      free (i);
      return NULL;
    }
  
  ch->data = (void *) i;
  ch->name = strdup (name);
  ch->location = NULL;
  ch->rate = rate;
  ch->channels = channels;
  
  /* Overrideable methods */

  ch->get_data = oss_mixer_channel_get_data;
  ch->free_data = oss_mixer_channel_free_data;

  ch->level = 20.0;
  ch->fade = 0.0000001;
  ch->fade_destination = 1.0;
  ch->outputs = NULL;
  ch->enabled = 1;
  ch->data_end_reached = 0;
  return ch;
}
