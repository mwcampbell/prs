#include <assert.h>
#include <stdio.h>
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
	} else {
		p->no_processing = 0;

		/* Allocate buffers */

		p->input_buffer_size = (latency/44100.0)*ch->rate;
		p->input_buffer = (SAMPLE *) malloc (p->input_buffer_size*sizeof(SAMPLE));
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
	short *input_position;
	short *output_position;
	int space_left;
	int input_samples;
	MixerChannel *ch = p->ch;
	MixerBus *bus = p->bus;
	double level;
	double fade;
	double fade_destination;
	
	/* Compute number of samples in the input */

	pthread_mutex_lock (&(ch->mutex));
	input_position = input;
	output_position = ch->output;
	space_left = ch->space_left;
	level = ch->level;
	fade = ch->fade;
	fade_destination = ch->fade_destination;
	pthread_mutex_unlock (&(ch->mutex));
	
	if (input_position >= output_position  && space_left > 0)
		input_samples = input_position-output_position;
	else
		input_samples = ch->buffer_end-output_position;
	input_samples /= ch->channels;
	if (input_samples >= ch->chunk_size)
		input_samples = ch->chunk_size;
	else if ((input_samples > 0 && space_left != ch->buffer_size) ||
		 (ch->data_reader_thread == -1 && input_samples == 0)) {
		ch->data_end_reached = 1;
	}
	else if (input_samples == 0)
		return;
	assert (p != NULL);

	if (level != 1.0 || fade != 1.0) {

                /* Fading */

		input = output_position;
		j = input_samples;
		while (j--) {
			*input *= level;
			input++;
			if (ch->channels == 2) {
				*input *= level;
				input++;
			}
			if ((fade < 1.0 && level <= fade_destination) ||
			    (fade > 1.0 && level >= fade_destination))
				fade = 1.0;
			if (fade != 1.0)
				level *= fade;
		}
	}
	if (p->no_processing) {
		mixer_bus_add_data (bus, output_position, input_samples);
	}

	else {
		input = output_position;
		i = input_samples*ch->channels;
		tmp = p->tmp_buffer;
		i = input_samples*ch->channels;
		j = 0;

		/*
		 *
		 * If we don't have a resampler, we're just downmixing stereo to mono
		 * or vice versa
		 *
		 */
	
		if (!p->resampler) {
			if (ch->channels > bus->channels)
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
			mixer_bus_add_data (bus, p->tmp_buffer, j/p->bus->channels);
			return;
		}
		else {
			sptr = p->input_buffer;
			if (ch->channels > bus->channels) {
				resample_channels =
					(ch->channels > bus->channels) ? bus->channels
					: ch->channels;
				
				while (i) {
					*sptr++ = (*input * (1.0f / 32768.0f) + *(input + 1) * (1.0f / 32768.0f)) / 2.0f;
					input += 2;
					i-= 2;
					j++;
				}
			}
			else {
				resample_channels = ch->channels;
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
			if (bus->channels > ch->channels)
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
			mixer_bus_add_data (bus, p->tmp_buffer, j/bus->channels);		
		}
	}
	output_position += input_samples*p->ch->channels;
	if (output_position >= ch->buffer_end)
		output_position = ch->buffer;
	space_left += input_samples;
	pthread_mutex_lock (&(ch->mutex));
	ch->output = output_position;
	ch->space_left = space_left;
	ch->level = level;
	ch->fade = fade;
	pthread_mutex_unlock (&(ch->mutex));
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
