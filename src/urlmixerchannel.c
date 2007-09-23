/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include "list.h"
#include "mixer.h"
#include "mp3header.h"



typedef enum {
	CHANNEL_TYPE_UNKNOWN,
	CHANNEL_TYPE_MP3,
	CHANNEL_TYPE_LAST
} channel_type;

typedef struct {
	channel_type type;
	pthread_t transfer_thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	
	/* Reference to owning MixerChannel */

	MixerChannel *ch;

	/* Sample rate info */

	int rate;
	int channels;

	char *url;
	char *transfer_url;
	char *archive_file_name;
	int archive_file_fd;
	
	int connected;
	int destroyed;
	int bytes_sent;
	int decoder_connected;

	/* Decoder process ID and input and output fds */

	int decoder_pid;
	int decoder_input_fd;
	int decoder_output_fd;
} channel_info;






static list *url_channels = NULL;



static void
broken_pipe_handler (int signum)
{
	list *tmp;

	for (tmp = url_channels; tmp; tmp = tmp->next) {
		MixerChannel *ch = (MixerChannel *) tmp->data;
		channel_info *i = (channel_info *) ch->data;
		ch->data_end_reached = 1;
		i->decoder_connected = 0;
	}
}



static void
channel_info_destroy (channel_info *i)
{
	list *tmp, *next;
	pthread_t transfer_thread;
	int decoder_fd;

	if (!i)
		return;

	/* Wait for curl to exit */

	i->destroyed = 1;
	if (i->decoder_connected) {
		if (i->decoder_pid != -1) {
			kill (i->decoder_pid, SIGTERM);
		}
	}
	pthread_mutex_lock (&(i->mutex));
	i->destroyed = 1;
	transfer_thread = i->transfer_thread;
	pthread_mutex_unlock (&(i->mutex));
	if (transfer_thread > 0)
		pthread_cancel (transfer_thread);
	
	pthread_mutex_lock (&(i->mutex));
	if (i->url)
		free (i->url);
	if (i->transfer_url)
		free (i->transfer_url);
	if (i->decoder_input_fd != -1)
		close (i->decoder_input_fd);
	if (i->decoder_output_fd != -1)
		close (i->decoder_output_fd);
	if (i->archive_file_fd != -1)
		close (i->archive_file_fd);

	/* Remove from our list of url channels */

	tmp = url_channels;
	while (tmp) {
		list *next = tmp->next;
		MixerChannel *ch = (MixerChannel *) tmp->data;
		if (ch == i->ch) {
			url_channels = prs_list_delete_item (url_channels, tmp);
		}
		tmp = next;
	}
	
	free (i);
}


static void
url_mixer_channel_free_data (MixerChannel *ch)
{
	channel_info *i = (channel_info *) ch->data;

	channel_info_destroy (i);
}


static int
url_mixer_channel_get_data (MixerChannel *ch)
{
	channel_info *i;
	int remainder;
	short *tmp;
	int rv;
  
	if (!ch)
		return;
	i = (channel_info *) ch->data;
	tmp = ch->input;
	remainder = ch->chunk_size*ch->channels;
	while (remainder > 0) {
		rv = read (i->decoder_output_fd, tmp, remainder*sizeof(short));
		if (rv == 0) {
			ch->data_end_reached = 1;
			break;
		}
		if (rv < 0) {
			rv = 0;
			break;
		}
		remainder -= rv/sizeof(short);
		tmp += rv/sizeof(short);
	}
	return ch->chunk_size-(remainder/ch->channels);
}


static size_t
curl_header_func (void *ptr,
		  size_t size,
		  size_t mem,
		  void *data)
{
	channel_info *i = (channel_info *) data;
	char *tmp, *tmp2;
	char *key;
	char *val;
	
	tmp = strstr ((char *) ptr, ": ");
	if (!tmp)
		return size*mem;
	key = strndup ((char *) ptr, tmp-(char *)ptr);
	tmp2 = strstr (tmp, "\r\n");
	if (tmp2)
		val = strndup (tmp+2, tmp2-tmp-2);
	else
		val = strdup (tmp+2);
	if (!strcmp (key, "Content-Type")) {
		if (!strcmp(val, "audio/mpeg"))
			i->type = CHANNEL_TYPE_MP3;
	}
	if (!strcmp (key, "Location")) {
		i->transfer_url = strdup (val);
	}
	if (key)
		free (key);
	if (val)
		free (val);
	return size*mem;
}



static int
start_mp3_decoder (channel_info *i)
{
	int input[2];
	int output[2];
	list *args_list = NULL;
	char **args_array;
	char *prog_name;
	int pid;
	
	/* Setup pipe */

	pipe (input);
	fcntl (input[1], F_SETFD, FD_CLOEXEC);
	pipe (output);
	fcntl (output[0], F_SETFD, FD_CLOEXEC);
	fcntl (output[0], F_SETFL, O_NONBLOCK);
	
	pid = fork ();
	if (pid == 0) {
		close (output[0]);
		close (input[1]);
		close (0);
		close (1);
		dup (input[0]);
		dup (output[1]);
		dup (output[1]);

		/* Setup program call */

		switch (i->type) {
		case CHANNEL_TYPE_MP3:
			prog_name = "mpg123";
			args_list = string_list_prepend (args_list, prog_name);
			args_list = string_list_prepend (args_list, "-s");
			args_list = string_list_prepend (args_list, "-q");
			args_list = string_list_prepend (args_list, "-");
			args_list = prs_list_reverse (args_list);
			args_array = string_list_to_array (args_list);
			prs_list_free (args_list);
			break;
		}
	
		execvp (prog_name, args_array);
	}
	/* ignore broken pipe  and SIGCHLD*/

	signal (SIGPIPE, SIG_IGN);
	signal (SIGCHLD, SIG_IGN);
	
	i->decoder_pid = pid;
	i->decoder_input_fd = input[1];
	i->decoder_output_fd = output[0];
	i->decoder_connected = 1;

	close (output[1]);
	close (input[0]);
	
	return 0;
}


static char *
mp3_process_first_block (channel_info *i,
			 void *ptr,
			 int size)
{
	unsigned long ulong_header = 0l;
	mp3_header_t mh;
	static mp3_header_t last_mh;
	unsigned char *buf, *end_buf;
	unsigned char *rv = NULL;

	buf = (unsigned char *) ptr;	

	end_buf = (char *) ptr+(size-4);
	while (buf <= end_buf) {
		if (*buf == 0XFF && (*(buf+1) >> 4) == 0X0F) {
			ulong_header = (((*buf) << 24) |
					(*(buf+1) << 16) |
					(*(buf+2) << 8) |
					(*(buf+3)));
			mp3_header_parse (ulong_header, &mh);
			if (mh.syncword == 4095 && mh.syncword == last_mh.syncword &&
			    mh.samplerate > 0 && mh.samplerate == last_mh.samplerate &&
			    mh.bitrate > 0 && mh.bitrate == last_mh.bitrate) {
			  rv = buf;
			  break;
			}
		last_mh.syncword = mh.syncword;
		last_mh.bitrate = mh.bitrate;
		last_mh.samplerate = mh.samplerate;
		}
		buf++;
	}
	
	if (buf > end_buf)
		return NULL;
		
	i->rate = mh.samplerate;
	i->channels = (mh.stereo > 1) ? 2 : 1;
	return rv;
}


static char *
process_icy_headers (channel_info *i,
		     char *buf)
{
	char *retval;

	i->type = CHANNEL_TYPE_MP3;
	retval = strstr (buf, "\r\n\r\n");
	if (retval)
		return retval+4;
	else
		return NULL;
}


static size_t
curl_write_func (void *ptr,
		 size_t size,
		 size_t mem,
		 void *data)
{
	channel_info *i = (channel_info *) data;
	char *buf = (char *) ptr;
	int bytes_to_process = mem*size;
	int bytes_sent;
	
	pthread_mutex_lock (&(i->mutex));
	if (i->destroyed) {
		pthread_mutex_unlock (&(i->mutex));
		return 0;
	}
	
        /* If this is the first block of data we're receiving, set the
	 * connected flag and signal any thread that might be waiting to know
	 * if we're connected
	 */

	if (!i->connected) {

		/* If this is any ICY header, process it */

		if (!strncmp(buf, "ICY", 3) ||
		    !strncmp (buf, "icy", 3)) {
			buf = process_icy_headers (i, buf);
			if (buf) 
				bytes_to_process -= (buf-(char *)ptr);
			if (!buf || bytes_to_process <= 0) {
				pthread_mutex_unlock (&(i->mutex));
				return mem*size;
			}
		}

		/* Do any special per type processing of the initial block */

		if (i->type == CHANNEL_TYPE_MP3) {
			char *rv = mp3_process_first_block (i,
							    buf,
							    bytes_to_process);
			if (!rv) {
				pthread_mutex_unlock (&(i->mutex));
				return mem*size;
			}
			buf = rv;
			bytes_to_process = (mem*size)-(buf-(char *)ptr);
			i->connected = 1;
			i->bytes_sent = 0;
			start_mp3_decoder (i);
		}
	}
	if (i->connected)
		bytes_sent = bytes_sent +  mem*size;
	if (i->bytes_sent <= 4096 && bytes_sent > 4096 && i->connected) {
		pthread_cond_broadcast (&(i->cond));
		if (i->archive_file_name) {
			i->archive_file_fd = open (i->archive_file_name, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
			if (i->archive_file_fd <= 0)
				i->archive_file_fd = -1;
		}
		else
			i->archive_file_fd = -1;
	}
	
	if (i->archive_file_fd != -1 && bytes_to_process > 0) {
		pthread_mutex_unlock (&(i->mutex));
		write (i->archive_file_fd, (void *) buf, bytes_to_process);
		pthread_mutex_lock (&(i->mutex));
	}
	if (i->decoder_connected && bytes_to_process > 0) {
		int rv;

		pthread_mutex_unlock (&(i->mutex));
		rv = write (i->decoder_input_fd, (void *) buf, bytes_to_process);
 		pthread_mutex_lock (&(i->mutex));
		if (rv <= 0) {
			if (errno == EPIPE)
				size = 0;
		}
	}
	i->bytes_sent = bytes_sent;
	pthread_mutex_unlock (&(i->mutex));
	return mem*size;
}


static void *
curl_transfer_thread_func (void *data)
{
	channel_info *i = (channel_info *) data;
	CURL *url;
	
	url = curl_easy_init ();
	curl_easy_setopt (url, CURLOPT_URL, i->url);
	curl_easy_setopt (url, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt (url, CURLOPT_WRITEFUNCTION, curl_write_func);
	curl_easy_setopt (url, CURLOPT_WRITEDATA, i);
	curl_easy_setopt (url, CURLOPT_HEADERFUNCTION, curl_header_func);
	curl_easy_setopt (url, CURLOPT_WRITEHEADER, i);
	curl_easy_setopt (url, CURLOPT_USERAGENT, "PRS 1.0");
	curl_easy_setopt (url, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
	curl_easy_setopt (url, CURLOPT_MAXREDIRS, 5);
	curl_easy_perform (url);
	curl_easy_cleanup (url);
	pthread_testcancel ();
	pthread_mutex_lock (&(i->mutex));
	if (i->connected) {
		if (i->ch && !i->destroyed) {
			i->ch->data_end_reached = 1;
			i->transfer_thread = 0;
			i->decoder_connected = 0;
		}
	}
	pthread_cond_broadcast (&(i->cond));
	pthread_mutex_unlock (&(i->mutex));
}


MixerChannel *
url_mixer_channel_new (const char *name,
		       const char *location,
		       const char *archive_file_name,
		       const int mixer_latency)
{
	MixerChannel *ch;
	int attempts = 0;
	channel_info *i;

	i = (channel_info *) calloc (sizeof(channel_info), 1);
	
	if (!i)
		return NULL;

	i->transfer_thread = 0;
	i->url = strdup (location);
	i->transfer_url = NULL;
	i->rate = -1;
	i->channels = -1;
	i->ch = NULL;
	
	/* Open archive file */

	if (archive_file_name) {
		i->archive_file_name = strdup (archive_file_name);
	}
	else
		i->archive_file_name = NULL;
	i->archive_file_fd = -1;
	
        /* Set pid and fds to invalid */

	i->decoder_pid = -1;
	i->decoder_input_fd = i->decoder_output_fd = -1;

        /* Flag indicating whether we're connected */

	i->connected = 0;
	
	/* Flag used to communicate destruction */

	i->destroyed = 0;

        /* Track number of byts sent */

	i->bytes_sent = 0;

        /* Flag indicating whether the decoder is connected */

	i->decoder_connected = 0;

        /* Create mutex and condition for notification when transfer begins */

        pthread_mutex_init (&(i->mutex), NULL);
	pthread_cond_init (&(i->cond), NULL);

        /* start transfer thread-- also handles redirects */

	while (!(i->connected) && attempts < 5) {
		pthread_mutex_lock (&(i->mutex));
		if (pthread_create (&(i->transfer_thread),
				    NULL,
				    curl_transfer_thread_func,
				    (void *) i)) {
			pthread_mutex_unlock (&(i->mutex));
			channel_info_destroy (i);
			return NULL;
		}
		
		/* the condition is signaled either when the transfer aborts, or\
		 * the first block of data is received
		 */
		
		pthread_cond_wait (&(i->cond), &(i->mutex));
		pthread_mutex_unlock (&(i->mutex));
		
		/* If we're not connected or we don't have sample rate parameters,
		 * either re-attempt to connect, or bail.
		 */

		if (!(i->transfer_url) && (!i->connected || i->rate == -1 ||
					   i->channels == -1)) {
			pthread_join (i->transfer_thread, NULL);
			channel_info_destroy (i);
			return NULL;
		}
		else if (i->transfer_url) {
			pthread_mutex_lock (&(i->mutex));
			free (i->url);
			i->url = i->transfer_url;
			i->transfer_url = NULL;
			i->bytes_sent = 0;
			pthread_mutex_unlock (&(i->mutex));
		}
		else
			break;
		attempts++;
	}
		
	ch = mixer_channel_new (i->rate, i->channels, mixer_latency);
	if (!ch) {
		channel_info_destroy (i);
		return NULL;
	}

	ch->name = strdup (name);
	ch->location = strdup (location);
	ch->data = (void *) i;
        pthread_mutex_lock (&(i->mutex));
        i->ch = ch;
        pthread_mutex_unlock (&(i->mutex));

	/* Overrideable methods */
	
	ch->get_data = url_mixer_channel_get_data;
	ch->free_data = url_mixer_channel_free_data;
	
	/* Add channel to global list of url channels */

	url_channels = prs_list_prepend (url_channels, ch);
	return ch;
}
