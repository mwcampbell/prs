#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/soundcard.h>
#include <shout/shout.h>
#include "shoutmixeroutput.h"



typedef struct {
  shout_conn_t *shout_connection;
  pid_t encoder_pid;
  int encoder_output_fd;
  int encoder_input_fd;
  int stream_reset;
  pthread_t shout_thread_id;
} shout_info;



static void
shout_conn_t_free (shout_conn_t *c)
{
  if (!c)
    return;
  if (c->ip)
    free (c->ip);
  if (c->mount)
    free (c->mount);
  if (c->password)
    free (c->password);
  if (c->aim)
    free (c->aim);
  if (c->icq)
    free (c->icq);
  if (c->irc)
    free (c->irc);
  if (c->dumpfile)
    free (c->dumpfile);
  if (c->name)
    free (c->name);
  if (c->url)
    free (c->url);
  if (c->genre)
    free (c->genre);
  if (c->description)
    free (c->description);
}




static void
start_encoder (MixerOutput *o)
{
  shout_info *i;
  int encoder_output[2];
  int encoder_input[2];

  if (!o)
    return;
  if (!o->data)
    return;
  i = (shout_info *) o->data;
  
  /* Create pipes */

  pipe (encoder_output);
  pipe (encoder_input);

  /* Fork the encoder process */

  i->encoder_pid = fork ();
  if (!i->encoder_pid)
    {
      char sample_rate_arg[128];
      char bitrate_arg[128];
      
      sprintf (sample_rate_arg, "-s %lf", (double) o->rate/1000);
      sprintf (bitrate_arg, "-b%d", i->shout_connection->bitrate);
      close (0);
      dup (encoder_input[0]);
      close (encoder_input[1]);
      
      close (1);
      dup (encoder_output[1]);
      close (encoder_output[0]);
      close (2);
      execlp ("lame",
	      "-r",
	      sample_rate_arg,
	      "-x",
	      "-a",
	      "-mm",
	      bitrate_arg,
	      "--resample",
	      "22.050",
	      "-",
	      "-",
	    NULL);
    }
  else
    {
      close (encoder_input[0]);
      i->encoder_input_fd = encoder_input[1];
      close (encoder_output[1]);
      i->encoder_output_fd = encoder_output[0];
    }
}



static void
stop_encoder (MixerOutput *o)
{
  shout_info *i;

  if (!o)
    return;
  if (!o->data)
    return;

  i = (shout_info *) o->data;
  close (i->encoder_input_fd);
  close (i->encoder_output_fd);
  waitpid (i->encoder_pid, NULL, 0);
}



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
    shout_conn_t_free (i->shout_connection);
  stop_encoder (o);
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



static void *
shout_thread (void *data)
{
  MixerOutput *o = (MixerOutput *) data;
  shout_info *i = (shout_info *) o->data;
  char buffer[1024];
  int bytes_read;

  
  if (!shout_connect (i->shout_connection))
    {
      fprintf (stderr, "Couldn'[t connect to icecast server.\n");
      o->enabled = 0;
      return;
    }
  fprintf (stderr, "Connected OK.\n");
  while (!i->stream_reset)
    {
      bytes_read = read (i->encoder_output_fd, buffer, 1024);
      if (bytes_read > 0)
	shout_send_data (i->shout_connection, buffer, bytes_read);
      shout_sleep (i->shout_connection);
    }
  fprintf (stderr, "Shout thread exiting...\n");
}



MixerOutput *
shout_mixer_output_new (const char *name,
			int rate,
			int channels,
			shout_conn_t *connection)
{
  MixerOutput *o;
  shout_info *i;
  
  if (!connection)
    return NULL;

  i = malloc (sizeof (shout_info));
  if (!i)
    return NULL;

  i->shout_connection = connection;
  o = malloc (sizeof (MixerOutput));
  if (!o)
    {
      free (i->shout_connection);
      free (i);
      return NULL;
    }
  
  o->name = strdup (name);
  o->enabled = 1;
  o->rate = rate;
  o->channels = channels;
  o->data = (void *) i;

  /* Overrideable methods */

  o->free_data = shout_mixer_output_free_data;
  o->post_output = shout_mixer_output_post_output;

  mixer_output_alloc_buffer (o);

  start_encoder (o);
  i->stream_reset = 0;
  pthread_create (&i->shout_thread_id, NULL, shout_thread, (void *) o);
  return o;
}



const shout_conn_t *
shout_mixer_output_get_connection (MixerOutput *o)
{
  shout_info *i;

  if (!o)
    return NULL;
  if (!o->data)
    return NULL;
  i = (shout_info *) o->data;
  return i->shout_connection;
}



void
shout_mixer_output_set_connection (MixerOutput *o,
				   const shout_conn_t *connection)
{
  shout_info *i;
  
  if (!o)
    return;
  if (!o->data)
    return;
  i = (shout_info *) o->data;

  /* Stop the encoder */

  stop_encoder (o);

  /* Signal the shoutcast thread that the stream has been reset, and wait
   * for it to exist
   */

  i->stream_reset = 1;
  pthread_join (i->shout_thread_id, NULL);

  /* Disconnect from shoutcast server */

  shout_disconnect (i->shout_connection);

  /* Free the shout_connection data structure */

  shout_conn_t_free (i->shout_connection);

  /* Wait a couple seconds before trying to re-connect */

  usleep (2000000);
  
  /* Set the shout_connection pointer to point to the new shout_conn_t structure */

  i->shout_connection = connection;


  /* The stream has been reset, so clear the reset flag */

  i->stream_reset = 0;

  /* Restart the encoder */

  start_encoder (o);


  /* Restart the shoutcast thread */

  pthread_create (&i->shout_thread_id, NULL, shout_thread, (void *) i);
}

