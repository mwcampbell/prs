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
 * ossmixeroutput.h: header file for  a mixer output which sends audio to an
 * oss soundcard.
 *
 */

#ifndef _OSS_MIXER_OUTPUT_H
#define _OSS_MIXER_OUTPUT_H
#include "mixeroutput.h"



MixerOutput *
oss_mixer_output_new (const char *name,
		      int rate,
		      int channels,
		      int latency);
#endif
