/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * mixerpatchpoint.h: Header file for the patch point object - connects a
 * mixer channel to a mixer bus.
 *
 */

#ifndef _MIXEr_PATCH_POINT_h
#define _MIXER_PATCH_POINT_H
#include "resample.h"
#include "mixerchannel.h"
#include "mixerbus.h"



typedef struct {
	int no_processing;
	res_state *resampler;
	MixerChannel *ch;
	MixerBus *bus;

	/* Buffers */

	short *tmp_buffer;
	int tmp_buffer_size;
	int tmp_buffer_length;
	SAMPLE *input_buffer;
	int input_buffer_size;
	int input_buffer_length;
	SAMPLE *output_buffer;
	int output_buffer_size;
	int output_buffer_length;
} MixerPatchPoint;



MixerPatchPoint *
mixer_patch_point_new (MixerChannel *ch,
		       MixerBus *b,
		       int latency);
void
mixer_patch_point_destroy (MixerPatchPoint *p);



#endif
