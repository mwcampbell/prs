#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include "ossmixeroutput.h"



typedef struct {
  int fd;
} oss_info;



static void
oss_mixer_output_free_data (MixerOutput *o)
{
  oss_info *i;

  if (!o->data)
    return;
  i = (oss_info *) o->data;
  close (i->fd);
  free (o->data);
}



static void
oss_mixer_output_post_output (MixerOutput *o)
{
  oss_info *i;
  
  if (!o)
    return;
  i = (oss_info *) o->data;

  write (i->fd, o->buffer, o->buffer_size*sizeof(short));
}



MixerOutput *
oss_mixer_output_new (const char *name,
		      int rate,
		      int channels)
{
  MixerOutput *o;
  oss_info *i;
  int tmp;
  
  i = malloc (sizeof (oss_info));
  if (!i)
    return NULL;

  /* Open the sound device */

  i->fd = open ("/dev/dsp", O_WRONLY);
  if (i->fd < 0)
    {
      free (i);
      return NULL;
      }

  /* Setup sound card */

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
  tmp = 0x8;
  if (ioctl (i->fd, SNDCTL_DSP_SETFRAGMENT, &tmp) < 0)
    {
      close (i->fd);
      free (i);
      return NULL;
    }
  
  o = malloc (sizeof (MixerOutput));
  if (!o)
    {
      close (i->fd);
      free (i);
      return NULL;
    }
  
  o->name = strdup (name);
  o->rate = rate;
  o->channels = channels;
  o->data = (void *) i;

  /* Overrideable methods */

  o->free_data = oss_mixer_output_free_data;
  o->post_output = oss_mixer_output_post_output;

  mixer_output_alloc_buffer (o);
  return o;
}
