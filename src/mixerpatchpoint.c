/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "mixerpatchpoint.h"



MixerPatchPoint *
mixer_patch_point_new (MixerChannel *ch,
		       MixerBus *b,
		       int latency)
{
	MixerPatchPoint *p = NULL;
	debug_printf (DEBUG_FLAGS_MIXER,
		      "creating patch point from %s to %s, latency=%d\n",
		      ch->name, b->name, latency);
	p = (MixerPatchPoint *) malloc (sizeof(MixerPatchPoint));
	assert (p != NULL);

	p->ch = ch;
	p->bus = b;
	p->input_buffer_length = p->input_buffer_size = 0;
	p->output_buffer_length = p->output_buffer_size = 0;
	p->tmp_buffer_length = p->tmp_buffer_size = 0;
	if (ch->rate == b->rate && ch->channels == b->channels) {
		p->no_processing = 1;
		p->tmp_buffer = NULL;
		p->input_buffer = NULL;
		p->output_buffer = NULL;
		p->resampler = NULL;
	}
	else {
		p->no_processing = 0;

		/* Allocate buffers */

		p->input_buffer_size = ch->chunk_size;
		p->input_buffer = (SAMPLE *) malloc (p->input_buffer_size*sizeof(SAMPLE)*b->channels);
		assert (p->input_buffer != NULL);
		p->output_buffer_size = p->tmp_buffer_size =
		  (latency/44100.0)*b->rate;
		p->output_buffer = (SAMPLE *) malloc (p->output_buffer_size*sizeof(SAMPLE)*b->channels);
		assert (p->output_buffer != NULL);
		p->tmp_buffer = (short *) malloc (p->tmp_buffer_size*sizeof(short)*b->channels);
		assert (p->tmp_buffer != NULL);
		if (ch->rate != b->rate) {
			p->resampler = (res_state *) malloc (sizeof(res_state));
			assert (p->resampler != NULL);
			res_init (p->resampler,
				  (ch->channels < b->channels)
				  ? ch->channels : b->channels,
				  b->rate, ch->rate, RES_END);
		}
		else
			p->resampler = NULL;		
	}
	return p;
}


void
mixer_patch_point_post_data (MixerPatchPoint *p)
{
	short *input;
	short *tmp;
	int i, j, resample_channels;
	SAMPLE *sptr;

	if (p->no_processing) {
		mixer_bus_add_data (p->bus, p->ch->output, p->ch->this_chunk_size);
	}

	else {
		input = p->ch->output;
		i = p->ch->this_chunk_size*p->ch->channels;
		tmp = p->tmp_buffer;
		j = 0;

		/*
		 *
		 * If we don't have a resampler, we're just downmixing stereo to mono
		 * or vice versa
		 *
		 */
	
		if (!p->resampler) {
			if (p->ch->channels > p->bus->channels)
				while (i) {
					*tmp++ = (int) (*input+*(input+1))/2;
					input += 2;
					i -= 2;
					j += 1;
				}
			else {
				while (i) {
					*tmp = *(tmp+1) = *input++;
					tmp += 2;
					i--;
					j += 2;
				}
			}
			p->tmp_buffer_length = j;
			mixer_bus_add_data (p->bus, p->tmp_buffer, j/p->bus->channels);
		}
		else {
			sptr = p->input_buffer;
			if (p->ch->channels > p->bus->channels) {
				resample_channels =
					(p->ch->channels > p->bus->channels) ? p->bus->channels
					: p->ch->channels;
				
				while (i) {
					*sptr++ = (*input * (1.0f / 32768.0f) + *(input + 1) * (1.0f / 32768.0f)) / 2.0f;
					input += 2;
					i-= 2;
					j++;
				}
			}
			else {
				resample_channels = p->ch->channels;
				while (i) {
					*sptr++ = *input++ * (1.0f / 32768.0f);
					i--;
					j++;
				}
			}
			i = p->output_buffer_length =
				res_push_interleaved (p->resampler,
						      p->output_buffer,
						      p->input_buffer,
						      j/resample_channels);
			i *= resample_channels;
			sptr = p->output_buffer;
			j = 0;
			tmp = p->tmp_buffer;
			if (p->bus->channels > p->ch->channels)
				while (i) {
					long sample = 32767 * (*sptr++);
					if (sample < -32768)
						sample = -32768;
					if (sample > 32767)
						sample = 32767;
					*tmp = *(tmp+1) = sample;
					tmp += 2;
					i--;
					j += 2;
				}
			else
				while (i) {
					long sample = 32767 * (*sptr++);
					if (sample < -32768)
						sample = -32768;
					if (sample > 32767)
						sample = 32767;
					*tmp++ = sample;
					j++;
					i--;
				}
			mixer_bus_add_data (p->bus, p->tmp_buffer, j/p->bus->channels);		
		}
	}
}


void
mixer_patch_point_destroy (MixerPatchPoint *p)
{
	assert (p != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_patch_point destroy called\n");
	if (p->resampler) {
		res_clear (p->resampler);
		free (p->resampler);
	}
	
	if (p->input_buffer)
		free (p->input_buffer);
	if (p->output_buffer)
		free (p->output_buffer);
	if (p->tmp_buffer)
		free (p->tmp_buffer);
	free (p);
}
