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
 * multibandaudiocompresor.h: header file for the multiband compressor.
 *
 */

#ifndef _MULTIBAND_AUDIO_COMPRESSOR_H
#define _MULTIBAND_AUDIO_COMPRESSOR_H
#include "audiofilter.h"

AudioFilter *
multiband_audio_compressor_new (int rate,
		      int channels,
		      int buffer_size);
void
multiband_audio_compressor_add_band (AudioFilter *f,
			   double freq,
			   double threshhold,
			   double ratio,
			   double attack_time,
			   double release_time,
			   double volume,
				     double output_gain);


#endif
