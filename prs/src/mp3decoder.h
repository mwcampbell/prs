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
 * mp3decoder.h: Header file for an object to wrap the use of mpg123.
 *
 */

#ifndef _MP3_DECODER_H_
#define _MP3_DECODER_H_



typedef struct _MP3Decoder MP3Decoder;



struct _MP3Decoder {
  int fd;
  int pid;
};



MP3Decoder *
mp3_decoder_new (const char *filename, int start_frame);
int
mp3_decoder_get_data (MP3Decoder *d, short *buffer, int size);
void
mp3_decoder_destroy (MP3Decoder *d);

#endif
