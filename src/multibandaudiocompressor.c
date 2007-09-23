/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "list.h"
#include "audiofilter.h"
#include "multibandaudiocompressor.h"


#define PI 3.1415926538979323846



typedef struct {
	float threshhold;
	short ithreshhold;
	float ratio;
	float attack_time;
	float release_time;
	float volume;
	float output_gain;  

	/* Filter stuff */

	float a0, a1, a2, b0, b1, b2;
	float x1[2];
	float y1[2];
	float x2[2];
	float y2[2];
  
	float link;

        /* Processing state variables */

	float level;
	float fade;
	float fade_destination;
} band;

static int
multiband_audio_compressor_process_data (AudioFilter *f,
					 short *input,
					 int input_length,
					 short *output,
					 int output_length)
{
	long long peak1, peak2;
	long prev_peak1, prev_peak2;
	float peak;
	short *buffer_end;
	short *iptr, *optr;
	list *tmp;
	int first_band = 1;
	band *b;

	assert (f != NULL);
	assert (input != NULL);
	assert (input_length >= 0);
	assert (output != NULL);
	assert (output_length >= 0);

	for (tmp = (list *) f->data; tmp; tmp = tmp->next) {
		b = (band *) tmp->data;

		/* Apply filter */

		buffer_end = input+input_length;      
		iptr = input;
		optr = f->buffer;
		while (iptr < buffer_end) {
			float in, out;
			long new_val;
			
			in = *iptr;
			out =
				(b->b0/b->a0)*in +
				(b->b1/b->a0)*b->x1[0] +
				(b->b2/b->a0)*b->x2[0] -
				(b->a1/b->a0)*b->y1[0] -
				(b->a2/b->a0)*b->y2[0];
			b->x2[0] = b->x1[0];
			b->x1[0] = in;
			b->y2[0] = b->y1[0];
			b->y1[0] = out;
			new_val = (*iptr)-out;
			if (new_val > 32767)
				new_val = 32767;
			else if (new_val < -32768)
				new_val = -32768;
			*iptr++ = new_val;
			out *= b->volume;
			if (out < -32768)
				out = -32768;
			else if (out > 32767)
				out = 32767;
			*optr++ = out;
			if (f->channels == 2)
			{
				in = *iptr;
				out =
					(b->b0/b->a0)*in +
					(b->b1/b->a0)*b->x1[1] +
					(b->b2/b->a0)*b->x2[1] -
					(b->a1/b->a0)*b->y1[1] -
					(b->a2/b->a0)*b->y2[1];
				b->x2[1] = b->x1[1];
				b->x1[1] = in;
				b->y2[1] = b->y1[1];
				b->y1[1] = out;
				new_val = (*iptr)-out;
				if (new_val > 32767)
					new_val = 32767;
				else if (new_val < -32768)
					new_val = -32768;
				*iptr++ = new_val;
				out *= b->volume;
				if (out > 32767)
					out = 32767;
				else if (out < -32768)
					out = -32768;
				*optr++ = out;
			}
		}
		buffer_end = f->buffer+f->buffer_size*f->channels;
		iptr = f->buffer;
		optr = output;
		while (iptr < buffer_end)
		{
			long out;

			if (first_band)
				out = *iptr++*b->level*b->output_gain;
			else
				out = (long)*optr+(long)*iptr++*b->level*b->output_gain;
			if (out < -32768)
				out = -32768;
			if (out > 32767)
				out = 32767;
			*optr++ = out;
			if (f->channels == 2) {
				if (first_band)
					out = *iptr++*b->level*b->output_gain;
				else
					out = (long)*optr+(long)*iptr++*b->level*b->output_gain;
				if (out < -32768)
					out = -32768;
				if (out > 32767)
					out = 32767;
				*optr++ = out;
			}
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
		peak1 = peak2 = 0;
		prev_peak1 = prev_peak2 = 0;
		iptr = f->buffer;
		buffer_end = f->buffer+f->buffer_size*f->channels;
		while (iptr < buffer_end)
		{
			long val = *iptr++;
			peak1 += val*val;
			if (f->channels == 2)
			{
				long val = *iptr++;
				peak2 += val*val;
			}
		}
		peak1 = sqrt(peak1);
		if (f->channels == 1)
			peak2 = peak1;
		else
			peak2 = sqrt(peak2);
		peak = ((peak1+peak2)/2)*(1-b->link)+
			((prev_peak1+prev_peak2)/2)*b->link;
		if (peak  > b->ithreshhold)
		{
			float peak_gain = log10 ((float)(peak1+peak2)/2/32767)*20;      
			float delta = b->threshhold-peak_gain;
			b->fade_destination = pow (10.0, (delta-delta/b->ratio)/20);
			if (b->fade_destination < b->level)
				b->fade = b->attack_time;
			else
				b->fade = b->release_time;
		}
		else if (b->fade_destination != 0)
		{
			b->fade_destination = 1;
			b->fade = b->release_time;
		}
		first_band = 0;
		prev_peak1 = peak1;
		prev_peak2 = peak2;
	}
	return input_length;
}



AudioFilter *
multiband_audio_compressor_new (int rate,
				int channels,
				int latency)
{
	AudioFilter *f = audio_filter_new (rate, channels, latency);

	f->data = NULL;
	f->process_data = multiband_audio_compressor_process_data;
	return f;
}



void
multiband_audio_compressor_add_band (AudioFilter *f,
				     float freq,
				     float threshhold,
				     float ratio,
				     float attack_time,
				     float release_time,
				     float volume,
				     float output_gain,
				     float link)
{
	float compression_amount;
	band *b ;
	list *bands;
	float omega = 2*PI*freq/f->rate;
	float sine = sin(omega);
	float cosine = cos(omega);
	float Q = 1;
	float alpha = sine/(2*Q);

	
	b = (band *) malloc (sizeof (band));

	b->link = link;

        /* Setup low-pass filter */

	b->b0 = (1-cosine)/2;
	b->b1 = 1-cosine;
	b->b2 = (1-cosine)/2;
	b->a0= 1+alpha;
	b->a1 = -2*cosine;
	b->a2 = 1-alpha;
		
        /* Initialize filters */

	b->x1[0] = b->x1[1] = 0.0;
	b->x2[0] = b->x2[1] = 0.0;
	b->y1[0] = b->y1[1] = 0.0;
	b->y2[0] = b->y2[1] = 0.0;

	/* Setup compressor threshhold */

	b->threshhold = threshhold;
	b->ithreshhold = pow (10.0, threshhold/20)*32767;

	/* Convert attack/release times to sample-based values */

	compression_amount = (threshhold-(threshhold/ratio));
	compression_amount = pow (10.0, compression_amount/20);  
	b->attack_time = pow (compression_amount, 1/(f->rate*attack_time));
	b->release_time = 1/pow (compression_amount, 1/(f->rate*release_time));
	b->ratio = ratio;
	b->volume = volume;
	b->output_gain = output_gain;


	/* Setup compressor state variables */

	b->level = 1.0;
	b->fade = 0.0;
	b->fade_destination = 1.0;

	bands = (list *) f->data;
	bands = prs_list_append (bands, b);
	f->data = bands;
}

			   
