#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "audiofilter.h"


typedef struct {
  double threshhold;
  double ratio;
  double attack_time;
  double release_time;
  double output_gain;
  
  /* Processing state variables */

  double level1;
  double level2;
  double fade1;
  double fade2;
  double fade_destination1;
  double fade_destination2;
} CompressorData;



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
  short ithreshhold;
  CompressorData *d = (CompressorData *) f->data;

  ithreshhold = pow (10.0, d->threshhold/20)*32767;
  peak1 = peak2 = 0;
  iptr = input;
  buffer_end = input+input_length;
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
  if (peak1 > ithreshhold)
    {
      double peak_gain = log10 ((double)peak1/32767)*20;      
      double delta = d->threshhold-peak_gain;
      d->fade_destination1 = delta-delta/d->ratio;
      d->fade1 = d->attack_time;
    }
  else if (d->fade_destination1 != 0)
    {
      d->fade_destination1 = 0;
      d->fade1 = d->release_time;
    }
  if (f->channels == 2)
    {
      if (peak2 > ithreshhold)
	{
	  double peak_gain = log10 ((double)peak2/32767)*20;
	  double delta = d->threshhold-peak_gain;
	  d->fade_destination2 = delta-delta/d->ratio;
	  d->fade2 = d->attack_time;
	}
      else if (d->fade_destination2 != 0)
	{
	  d->fade_destination2 = 0;
	  d->fade2 = d->release_time;
	}
    }
  iptr = input;
  optr = output;
  while (iptr < buffer_end)
    {
      double fl = pow (10.0, d->level1/20);
      *optr++ = *iptr++*fl*d->output_gain;
      if (d->fade1 != 0)
	{
	  if ((d->fade1 > 0 && d->level1 >= d->fade_destination1)
	      || (d->fade1 < 0 && d->level1 <= d->fade_destination1))
	    d->fade1 = 0.0;
	  else
	    d->level1 += d->fade1;
	}
      if (f->channels == 2)
	{
	  double fl = pow (10.0, d->level2/20);
	  *optr++ = *iptr++*fl*d->output_gain;
	  if (d->fade2 != 0)
	    {
	      if ((d->fade2 > 0 && d->level2 >= d->fade_destination2)
		  || (d->fade2 < 0 && d->level2 <= d->fade_destination2))
		d->fade2 = 0.0;
	      else
		d->level2 += d->fade2;
	    }
	}
    }
  return input_length;
}



AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int buffer_size,
		      double threshhold,
		      double ratio,
		      double attack_time,
		      double release_time,
		      double output_gain)
{
  AudioFilter *f = audio_filter_new (rate, channels, buffer_size);
  CompressorData *d = malloc (sizeof (CompressorData));
  d->attack_time = attack_time;
  d->release_time = release_time;
  d->fade1 = d->fade2 = 0.0;
  d->level1 = d->level2 = 0;
  d->fade_destination1 = d->fade_destination2 = 0;
  d->ratio = ratio;
  d->threshhold = threshhold;
  d->output_gain = output_gain;
  
  /* Convert attack/release times to sample-based values */

  d->attack_time = (threshhold-(threshhold/ratio))/(f->rate*attack_time);
  d->release_time = -(threshhold-(threshhold/ratio))/(f->rate*release_time);
  f->data = d;
  f->process_data = audio_compressor_process_data;
  return f;
}
