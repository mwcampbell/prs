#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "mp3decoder.h"



MP3Decoder *
mp3_decoder_new (const char *filename, int start_frame)
{
  MP3Decoder *d = NULL;
  int decoder_output[2] = {-1, -1};
  int rv = -1;
  char start_frame_str[10] = "0";

  if (!filename)
    return NULL;
  sprintf (start_frame_str, "%d", start_frame);

  /* Create pipe */

  if (pipe (decoder_output) < 0)
    return NULL;

  /* Fork the decoder process */

  signal (SIGCHLD, SIG_IGN);
  rv = fork ();
  if (!rv)
    {
      close (0);
      close (1);
      close (2);
      dup2 (decoder_output[1], 1);
      close (decoder_output[0]);
      execlp ("mpg123", "mpg123", "-q", "-s", "-k", start_frame_str,
	      filename, NULL);
      exit (1);
    }
  else if (rv > 0)
    {
      close (decoder_output[1]);
      d = malloc (sizeof (MP3Decoder));
      d->fd = decoder_output[0];
      d->pid = rv;
    }

  return d;
}



int
mp3_decoder_get_data (MP3Decoder *d, short *buffer, int length)
{
  int samples_left = length;
  short *ptr = buffer;
  int samples_read;
 
  while (samples_left > 0)
    {
      samples_read = read (d->fd, ptr, samples_left * sizeof (short));
      if (samples_read <= 0)
	break;
      samples_read /= sizeof (short);
      samples_left -= samples_read;
      ptr += samples_read;
    }
  if (samples_left)
    return length - samples_left;
  else
    return length;
}



void
mp3_decoder_destroy (MP3Decoder *d)
{
  if (!d)
    return;

  close (d->fd);
  free (d);
}
