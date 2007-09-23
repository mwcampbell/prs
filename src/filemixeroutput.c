/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "wave.h"
#include "filemixeroutput.h"


typedef struct {
	riff_header riff;
	chunk_header format_chunk_header;
	format_header format_chunk;
	chunk_header data_chunk_header;
	unsigned long size;
	int fd;
} file_info;



static void
file_mixer_output_free_data (MixerOutput *o)
{
	file_info *i;
	int tmp;
  
	if (!o)
		return;
	if (!o->data)
		return;
	i = (file_info *) o->data;
	
	/* Write headers */

	i->data_chunk_header.len = i->size;
	i->riff.len = i->size+2*sizeof(chunk_header)+sizeof(format_header);
	tmp = lseek (i->fd, 0l, SEEK_SET);
	fprintf (stderr, "Seekedg to 0l reported %d.\n", tmp);
	write (i->fd, &(i->riff), sizeof(riff_header));
	write (i->fd, &(i->format_chunk_header), sizeof(chunk_header));
	write (i->fd, &(i->format_chunk), sizeof(format_header));
	write (i->fd, &(i->data_chunk_header), sizeof(chunk_header));
	close (i->fd);
	free (o->data);
}



static void
file_mixer_output_post_data (MixerOutput *o)
{
	file_info *i;
  
	if (!o)
		return;
	i = (file_info *) o->data;

	i->size += o->buffer_size*sizeof(short);
	write (i->fd, o->buffer, o->buffer_size*sizeof(short));
}



MixerOutput *
file_mixer_output_new (const char *name,
		       int rate,
		       int channels,
		       int latency)
{
	MixerOutput *o;
	file_info *i;
	int tmp;
  
	i = malloc (sizeof (file_info));
	if (!i)
		return NULL;
	memset (i, 0, sizeof(file_info));
	
	i->fd = open (name, O_RDWR | O_CREAT,
		      S_IRWXU);
	lseek (i->fd,
	       (long) sizeof(riff_header)+2*sizeof(chunk_header)+sizeof(format_header), SEEK_SET);
	if (i->fd < 0) {
		free (i);
		return NULL;
	}

	/* Leave room for the header */

	o = malloc (sizeof (MixerOutput));
	if (!o) {
		close (i->fd);
		free (i);
		return NULL;
	}
	
	o->name = strdup (name);
	o->rate = rate;
	o->channels = channels;
	o->data = (void *) i;
	o->enabled = 1;
	
	/* Overrideable methods */

	o->free_data = file_mixer_output_free_data;
	o->post_data = file_mixer_output_post_data;

	/* Initialize RIFF HEADER */

	strncpy ((i->riff.riff_id), "RIFF", 4);
	
	/* Length is unknown */

	strncpy ((i->riff.type_id), "WAVE", 4);

	/* Initialize format chunk */

	strncpy ((i->format_chunk_header.id), "fmt ", 4);
	i->format_chunk_header.len = sizeof(format_header);
	i->format_chunk.format_tag = WAVE_FORMAT_PCM;
	i->format_chunk.channels = channels;
	i->format_chunk.samples_per_second = rate*channels;
	i->format_chunk.average_bytes_per_sec=2*rate*channels;
	i->format_chunk.block_align = 2*channels;
	i->format_chunk.bits_per_sample = 16;

	/* Initialize data chunk */

	strncpy ((i->data_chunk_header.id), "data", 4);
	
	/* Data length is unknown */

	i->size = 0l;
	mixer_output_alloc_buffer (o, latency);  
	return o;
}



