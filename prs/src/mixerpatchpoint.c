#include <stdio.h>
#include "mixerpatchpoint.h"



MixerPatchPoint *
mixer_patch_point_new (MixerChannel *ch,
		       MixerBus *b,
		       int latency)
{
	MixerPatchPoint *p = (MixerPatchPoint *) malloc (sizeof(MixerPatchPoint));
	if (!p)
		return NULL;

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

		p->input_buffer_size = latency/(88200/(ch->rate*ch->channels));
		p->input_buffer = (SAMPLE *) malloc (p->input_buffer_size*sizeof(SAMPLE));
		p->output_buffer_size = p->tmp_buffer_size =
			latency/(88200/(b->rate*b->channels));
		p->output_buffer = (SAMPLE *) malloc (p->output_buffer_size*sizeof(SAMPLE));
		p->tmp_buffer = (short *) malloc (p->tmp_buffer_size*sizeof(short));
		if (ch->rate != b->rate) {
			p->resampler = (res_state *) malloc (sizeof(res_state));
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

	if (!p)
		return;
	if (p->no_processing) {
		mixer_bus_add_data (p->bus, p->ch->buffer, p->ch->buffer_length);
		return;
	}

	input = p->ch->buffer;
	tmp = p->tmp_buffer;
	i = p->ch->buffer_length;
	j = 0;

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
		mixer_bus_add_data (p->bus, p->tmp_buffer, j);
		return;
	}
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
	} else {
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
	mixer_bus_add_data (p->bus, p->tmp_buffer, j);		
}


void
mixer_patch_point_destroy (MixerPatchPoint *p)
{
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
