/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
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
mp3_decoder_new (const char *filename, double start_time, int channels);
int
mp3_decoder_get_data (MP3Decoder *d, short *buffer, int size);
void
mp3_decoder_destroy (MP3Decoder *d);

#endif
