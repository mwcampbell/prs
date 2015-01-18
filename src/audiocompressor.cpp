/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <math.h>
extern "C" {
#include "debug.h"
#include "audiocompressor.h"
}
#include "SimpleComp.h"


typedef struct {
	chunkware_simple::SimpleComp comp;
	float output_gain;
} CompressorData;



static int
audio_compressor_process_data (AudioFilter *f,
			       short *input,
			       int input_length,
			       short *output,
			       int output_length)
{
	assert (f != NULL);
	assert (input != NULL);
	assert (input_length >= 0);
	assert (output != NULL);
	assert (output_length >= 0);
	CompressorData *d = (CompressorData *) f->data;
	short *iptr = input;
	short *optr = output;
	short *buffer_end = input+input_length;
	while (iptr < buffer_end) {
		double sample1, sample2;
		sample1 = (*iptr++) / 32768.0;
		if (f->channels == 2) {
			sample2 = (*iptr++) / 32768.0;
		} else {
			sample2 = sample1;
		}
		d->comp.process (sample1, sample2);
		long lsample = long(sample1 * d->output_gain * 32767);
		if (lsample < -32768) {
			lsample = -32768;
		} else if (lsample > 32767) {
			lsample = 32767;
		}
		*optr++ = lsample;
		if (f->channels == 2) {
			lsample = long(sample2 * d->output_gain * 32767);
			if (lsample < -32768) {
				lsample = -32768;
			} else if (lsample > 32767) {
				lsample = 32767;
			}
			*optr++ = lsample;
		}
	}
	return input_length;
}



AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int latency,
		      float threshhold,
		      float ratio,
		      float attack_time,
		      float release_time,
		      float output_gain)
{
	AudioFilter *f = NULL;
	CompressorData *d = NULL;

	debug_printf (DEBUG_FLAGS_MIXER,
		      "audio_compressor_new(%d, %d, %d, %f, %f, %f, %f, %f)\n",
		      rate, channels, latency, threshhold, ratio, attack_time,
		      release_time, output_gain);
	assert (rate > 0);
	assert ((channels == 1) || (channels == 2));
	assert (latency > 0);
	f = audio_filter_new (rate, channels, latency);
	d = new CompressorData;
	assert (d != NULL);
	d->comp.setSampleRate (rate);
	d->comp.setAttack (attack_time * 1000.0);
	d->comp.setRelease (release_time * 1000.0);
	d->comp.setThresh (threshhold);
	d->comp.setRatio (ratio);
	d->comp.initRuntime ();
	d->output_gain = output_gain;
  
	f->data = d;
	f->process_data = audio_compressor_process_data;
	return f;
}
