#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <shout/shout.h>
#include "shoutmixeroutput.h"



typedef struct {
  shout_conn_t *shout_connection;
  pid_t encoder_pid;
  int encoder_output_fd;
  int encoder_input_fd;
pthread_t shout_thread_id;
} shout_info;



static void
shout_mixer_output_free_data (MixerOutput *o)
{
  shout_info *i;

  if (!o)
    return;
  if (!o->data)
    return;
  i = (shout_info *) o->data;

  if (i->shout_connection)
    {
      if (i->shout_connection->ip)
	free (i->shout_connection->ip);
    if (i->shout_connection->password)
      free (i->shout_connection->password);
    }
  close (i->encoder_input_fd);
  waitpid (i->encoder_pid, NULL, 0);
  kill (i->encoder_pid);
  free (o->data);
}



static void
shout_mixer_output_post_output (MixerOutput *o)
{
  shout_info *i;
  
  if (!o)
    return;
  i = (shout_info *) o->data;

  write (i->encoder_input_fd, o->buffer, o->buffer_size*sizeof(short));
}



void *
shout_thread (void *data)
{
  shout_info *i = (shout_info *) data;
  char buffer[1024];
  int bytes_read;

  
  if (!shout_connect (i->shout_connection))
    {
      printf ("Couldn'[t connect to icecast server.\n");
      return;
      }
  printf ("Connected OK.\n");
  printf ("Starting shoutcast thread.\n");
  while (1)
    {
      bytes_read = read (i->encoder_output_fd, buffer, 1024);
      shout_send_data (i->shout_connection, buffer, bytes_read);
      shout_sleep (i->shout_connection);
    }
}



MixerOutput *
shout_mixer_output_new (const char *name,
		      int rate,
		      int channels,
			const char *ip,
			int port,
			const char *password)
{
  MixerOutput *o;
  shout_info *i;
  int encoder_output[2];
  int encoder_input[2];
  
  i = malloc (sizeof (shout_info));
  if (!i)
    return NULL;

  i->shout_connection = (shout_conn_t *) malloc (sizeof (shout_conn_t));
  memset (i->shout_connection, 0, sizeof (shout_conn_t));
  if (!i->shout_connection)
    {
      free (i);
      return;
      }
  if (ip)
    i->shout_connection->ip = strdup (ip);
  else
    i->shout_connection->ip = NULL;
  if (password)
    i->shout_connection->password = strdup (password);
  else
    i->shout_connection->password = NULL;
  i->shout_connection->port = port;
  o = malloc (sizeof (MixerOutput));
  if (!o)
    {
      free (i->shout_connection);
      free (i);
      return NULL;
    }
  
  o->name = strdup (name);
  o->rate = rate;
  o->channels = channels;
  o->data = (void *) i;

  /* Overrideable methods */

  o->free_data = shout_mixer_output_free_data;
  o->post_output = shout_mixer_output_post_output;

  mixer_output_alloc_buffer (o);

  /* Create pipes */

  pipe (encoder_output);
  pipe (encoder_input);


  /* Fork the encoder process */

  i->encoder_pid = fork ();
  if (!i->encoder_pid)
    {
      char sample_rate_arg[128];
      sprintf (sample_rate_arg, "-s %lf", (double) o->rate/1000);
      close (0);
      dup (encoder_input[0]);
      close (1);
      dup (encoder_output[1]);
      execlp ("lame",
	      "-r",
	      sample_rate_arg,
	      "-x",
	      "-a",
	      "-mm",
	      "-b24",
	      "-",
	      "-",
	    NULL);
    }
  else
    {
      close (encoder_input[0]);
      i->encoder_input_fd = encoder_input[1];
      i->encoder_output_fd = encoder_output[0];
      pthread_create (&i->shout_thread_id, NULL, shout_thread, (void *) i);
    }
  return o;
}
