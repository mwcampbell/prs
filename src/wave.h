/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
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
