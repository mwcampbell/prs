#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "filemixeroutput.h"


typedef struct {
  int fd;
} file_info;



static void
file_mixer_output_free_data (MixerOutput *o)
{
  file_info *i;
  int tmp;
  
  if (!o)
    return;
  if (!o->data)
    return;
  i = (file_info *) o->data;
  close (i->fd);
  free (o->data);
}



static void
file_mixer_output_post_data (MixerOutput *o)
{
  file_info *i;
  
  if (!o)
    return;
  i = (file_info *) o->data;

  write (i->fd, o->buffer, o->buffer_size*sizeof(short));
}



MixerOutput *
file_mixer_output_new (const char *name,
		      int rate,
		      int channels,
		       int latency)
{
  MixerOutput *o;
  file_info *i;
  int tmp;
  
  i = malloc (sizeof (file_info));
  if (!i)
    return NULL;

  i->fd = open (name, O_RDWR | O_CREAT);
  if (i->fd < 0)
    {
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
  o->enabled = 1;
  
  /* Overrideable methods */

  o->free_data = file_mixer_output_free_data;
  o->post_data = file_mixer_output_post_data;

  mixer_output_alloc_buffer (o, latency);  
  return o;
}



