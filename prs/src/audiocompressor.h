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
 * audiocompressor.h: header for the wideband audio compressor.
 *
 */

#ifndef _AUDIO_COMPRESSOR_H
#define _AUDIO_COMPRESSOR_H
#include "audiofilter.h"

AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int buffer_size,
		      double threshhold,
		      double ratio,
		      double attack_time,
		      double release_time,
		      double output_gain);


#endif
