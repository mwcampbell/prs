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
 * wave.h: Definitions of wave file header data structures.
 *
 */

#define WAVE_FORMAT_PCM 0X0001

typedef struct {
	unsigned char riff_id[4];
	unsigned int len;
	unsigned char type_id[4];
} riff_header;

typedef struct {
	unsigned char id[4];
	unsigned int len;
} chunk_header;



typedef struct {
	unsigned short format_tag;
	unsigned short channels;
	unsigned int samples_per_second;
	unsigned int average_bytes_per_sec;
	unsigned short block_align;
	unsigned short bits_per_sample;
} format_header;


#define WAVE_HEADER_SIZE sizeof(riff_header)+2*sizeof(chunk_header)+sizeof(format_header)
