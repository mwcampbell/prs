#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "debug.h"
#include "audiofilter.h"


typedef struct {
	double threshhold;
	short ithreshhold;
	double ratio;
	double attack_time;
	double release_time;
	double output_gain;
  
	/* Processing state variables */

	double level;
	double fade;
	double fade_destination;
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
	CompressorData *d = NULL;

	assert (f != NULL);
	assert (input != NULL);
	assert (input_length >= 0);
	assert (output != NULL);
	assert (output_length >= 0);
	d = (CompressorData *) f->data;
	peak1 = peak2 = 0;
	iptr = input;
	buffer_end = input+input_length;
	while (iptr < buffer_end) {
		short val = abs (*iptr++);
		if (val > peak1)
			peak1 = val;
		if (f->channels == 2) {
			short val = abs (*iptr++);
			if (val > peak2)
				peak2 = val;
		}
	}
	if ((double)(peak1+peak2)/2 > d->ithreshhold) {
		double peak_gain = log10 ((double)(peak1+peak2)/2/32767)*20;
		double delta = d->threshhold-peak_gain;
		d->fade_destination = pow (10.0, (delta-delta/d->ratio)/20);
		if (d->fade_destination < d->level)
			d->fade = d->attack_time;
		else
			d->fade = d->release_time;
	} else if (d->fade_destination != 0) {
		d->fade_destination = 1;
		d->fade = d->release_time;
	}
	iptr = input;
	optr = output;
	while (iptr < buffer_end) {
		*optr++ = *iptr++*d->level*d->output_gain;
		if (d->fade != 0) {
			if ((d->fade > 1 && d->level >= d->fade_destination) ||
			    (d->fade < 1 && d->level <= d->fade_destination)) {
				d->level = d->fade_destination;
				d->fade = 0.0;
			}
			else
				d->level *= d->fade;
		}
		if (f->channels == 2)
			*optr++ = *iptr++*d->level*d->output_gain;
	}
	return input_length;
}



AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int latency,
		      double threshhold,
		      double ratio,
		      double attack_time,
		      double release_time,
		      double output_gain)
{
	AudioFilter *f = NULL;
	CompressorData *d = NULL;
	double compression_amount;

	debug_printf (DEBUG_FLAGS_MIXER,
		      "audio_compressor_new(%d, %d, %d, %f, %f, %f, %f, %f)\n",
		      rate, channels, latency, threshhold, ratio, attack_time,
		      release_time, output_gain);
	assert (rate > 0);
	assert ((channels == 1) || (channels == 2));
	assert (latency > 0);
	f = audio_filter_new (rate, channels, latency);
	d = malloc (sizeof (CompressorData));
	assert (d != NULL);
	d->attack_time = attack_time;
	d->release_time = release_time;
	d->fade = 0.0;
	d->level = 1;
	d->fade_destination = 1;
	d->ratio = ratio;
	d->threshhold = threshhold;
	d->ithreshhold = pow (10.0, threshhold/20)*32767;
  
	/* Convert attack/release times to sample-based values */

	compression_amount = (threshhold-(threshhold/ratio));
	compression_amount = pow (10.0, compression_amount/20);
	d->attack_time = pow (compression_amount, 1/(rate*attack_time));
	d->release_time = 1/pow (compression_amount, 1/(rate*release_time));
	d->output_gain = output_gain;
  
	f->data = d;
	f->process_data = audio_compressor_process_data;
	return f;
}
