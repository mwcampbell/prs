#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "list.h"
#include "audiofilter.h"


#define PI 3.1415926538979323846



typedef struct {
  double threshhold;
  short ithreshhold;
  double ratio;
  double attack_time;
  double release_time;
  double output_gain;  

  /* Filter stuff */

  double a1[3];
  double a2[3];
  double b1[2];
  double b2[2];
  double x1[2];
  double y1[2];
  double x2[2];
  double y2[2];
  double x3[2];
  double y3[2];
  double x4[2];
  double y4[2];
  
  /* Processing state variables */

  double level;
  double fade;
  double fade_destination;
} band;

static int
audio_compressor_process_data (AudioFilter *f,
			       short *input,
			       int input_length,
			       short *output,
			       int output_length)
{
  short peak1, peak2;
  short *buffer_end;
  short *iptr, *optr;
  list *tmp;
  int first_band = 1;
  band *b;
  
  for (tmp = (list *) f->data; tmp; tmp = tmp->next)
    {
      b = (band *) tmp->data;

      /* Apply filter */

      /* Pre-process the buffer to give the filters some head room */

      iptr = input;
      buffer_end = input+input_length;
      while (iptr < buffer_end)
	*iptr++ *= .7;
      iptr = input;
      optr = f->buffer;
      while (iptr < buffer_end)
	{
	  double in, out;

	  in = *iptr;
	  out =
	    b->a1[0]*in +
	    b->a1[1]*b->x1[0] +
	    b->a1[2]*b->x1[1] -
	    b->b1[0]*b->y1[0] -
	    b->b1[1]*b->y1[1];
	  if (out < -32768)
	    out = -32768;
	  if (out > 32767)
	    out = 32767;
	  b->x1[1] = b->x1[0];
	  b->x1[0] = in;
	  b->y1[1] = b->y1[0];
	  b->y1[0] = out;
	  *optr++ = out;
	  out =
	    b->a2[0]*in +
	    b->a2[1]*b->x3[0] +
	    b->a2[2]*b->x3[1] -
	    b->b2[0]*b->y3[0] -
	    b->b2[1]*b->y3[1];
	  if (out < -32768)
	    out = -32768;
	  if (out > 32767)
	    out = 32767;
	  b->x3[1] = b->x3[0];
	  b->x3[0] = in;
	  b->y3[1] = b->y3[0];
	  b->y3[0] = out;
	  *iptr++ = out;
	  if (f->channels == 2)
	    {
	      in = *iptr;
	      out =
		b->a1[0]*in +
		b->a1[1]*b->x2[0] +
		b->a1[2]*b->x2[1] -
		b->b1[0]*b->y2[0] -
		b->b1[1]*b->y2[1];
	      if (out < -32768)
		out = -32768;
	      if (out > 32767)
		out = 32767;
	      b->x2[1] = b->x2[0];
	      b->x2[0] = in;
	      b->y2[1] = b->y2[0];
	      b->y2[0] = out;
	      *optr++ = out;
	      out =
		b->a2[0]*in +
		b->a2[1]*b->x4[0] +
		b->a2[2]*b->x4[1] -
		b->b2[0]*b->y4[0] -
		b->b2[1]*b->y4[1];
	      if (out < -32768)
		out = -32768;
	      if (out > 32767)
		out = 32767;
	      b->x4[1] = b->x4[0];
	      b->x4[0] = in;
	      b->y4[1] = b->y4[0];
	      b->y4[0] = out;
	      *iptr++ = out;
	    }
	  }
      peak1 = peak2 = 0;
      iptr = f->buffer;
      buffer_end = f->buffer+f->buffer_size;
      while (iptr < buffer_end)
	{
	  short val = abs (*iptr++);
	  if (val > peak1)
	    peak1 = val;
	  if (f->channels == 2)
	    {
	      short val = abs (*iptr++);
	      if (val > peak2)
		peak2 = val;
	    }
	}
      if ((double) (peak1+peak2)/2  > b->ithreshhold)
	{
	  double peak_gain = log10 ((double)(peak1+peak2)/2/32767)*20;      
	  double delta = b->threshhold-peak_gain;
	  b->fade_destination = pow (10.0, (delta-delta/b->ratio)/20);
	  b->fade = b->attack_time;
	}
      else if (b->fade_destination != 0)
	{
	  b->fade_destination = 1;
	  b->fade = b->release_time;
	}
      iptr = f->buffer;
      optr = output;
      while (iptr < buffer_end)
	{
	  if (first_band)
	    *optr++ = *iptr++*b->level*b->output_gain;
	  else
	    *optr++ += *iptr++*b->level*b->output_gain;
	  if (f->channels == 2)
	    if (first_band)
	      *optr++ = *iptr++*b->level*b->output_gain;
	    else
	      *optr++ += *iptr++*b->level*b->output_gain;
	  if (b->fade != 0)
	    {
	      if ((b->fade > 1 && b->level > b->fade_destination)
		  || (b->fade < 1 && b->level < b->fade_destination))
		{
		  b->fade = 0.0;
		}
	      else
		b->level *= b->fade;
	    }
	}
      first_band = 0;
    }
  return input_length;
}



AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int buffer_size)
{
  AudioFilter *f = audio_filter_new (rate, channels, buffer_size);

  f->data = NULL;
  f->process_data = audio_compressor_process_data;
  return f;
}



void
audio_compressor_add_band (AudioFilter *f,
			   double freq,
			   double threshhold,
			   double ratio,
			   double attack_time,
			   double release_time,
			   double output_gain)
{
  double compression_amount;
  band *b ;
  list *bands;
  double c;
  
  b = (band *) malloc (sizeof (band));

  /* Setup low-pass filter */

  c = 1.0/tan(PI*freq/f->rate);
  b->a1[0] = 1.0/(1.0+sqrt(2)*c+c*c);
  b->a1[1] = 2.0*b->a1[0];
  b->a1[2] = b->a1[0];
  b->b1[0] = 2.0*(1.0-c*c)*b->a1[0];
  b->b1[1] = (1.0-sqrt(2.0)*c+c*c)*b->a1[0];

  /* Setup high-pass filter */

  c = tan (PI*freq/f->rate);
  b->a2[0] = 1/(1.0+sqrt(2)*c+c*c);
  b->a2[1] = -2.0*b->a2[0];
  b->a2[2] = b->a2[0];
  b->b2[0] = 2*(c*c-1.0)*b->a2[0];
  b->b2[1] = (1.0-sqrt(2.0)*c+c*c)*b->a2[0];

  /* Initialize filters */

  b->x1[0] = 0.0;
  b->x1[1] = 0.0;
  b->x2[0] = 0.0;
  b->x2[1] = 0.0;
  b->x3[0] = 0.0;
  b->x3[1] = 0.0;
  b->x4[0] = 0.0;
  b->x4[1] = 0.0;
  b->y1[0] = 0.0;
  b->y1[1] = 0.0;
  b->y2[0] = 0.0;
  b->y2[1] = 0.0;
  b->y3[0] = 0.0;
  b->y3[1] = 0.0;
  b->y4[0] = 0.0;
  b->y4[1] = 0.0;

  /* Setup compressor threshhold */

  b->threshhold = threshhold;
  b->ithreshhold = pow (10.0, threshhold/20)*32767;

  /* Convert attack/release times to sample-based values */

  compression_amount = (threshhold-(threshhold/ratio));
  compression_amount = pow (10.0, compression_amount/20);  
  b->attack_time = pow (compression_amount, 1/(f->rate*attack_time));
  b->release_time = 1/pow (compression_amount, 1/(f->rate*release_time));
  b->ratio = ratio;
  b->output_gain = output_gain;


  /* Setup compressor state variables */

  b->level = 1.0;
  b->fade = 0.0;
  b->fade_destination = 1.0;

  bands = (list *) f->data;
  bands = list_append (bands, b);
  f->data = bands;
}

			   
