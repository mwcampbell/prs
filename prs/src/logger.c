/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include "fileinfo.h"
#include "logger.h"


logger *
logger_new (const LOGGER_TYPE type,
	    const char *log_file_name,
	    const char *url,
	    const char *username,
	    const char *password)
{
	logger *l = malloc (sizeof(logger));

	if (log_file_name) {
		l->log_file = fopen (log_file_name, "w");
		fprintf (l->log_file, "<log>\n");
	}
	else
		l->log_file = NULL;
	l->type = type;
	if (url)
		l->url = strdup (url);
	else
		l->url = NULL;
	if (username)
		l->username = strdup (username);
	else
		l->username = NULL;
	if (password)
		l->password = strdup (password);
	else
		l->password = NULL;
	return l;
}


void
logger_destroy (logger *l)
{
	if (!l)
		return;
	if (l->url)
		free (l->url);
	if (l->username)
		free (l->username);
	if (l->password)
		free (l->password);
	if (l->log_file) {
		fprintf (l->log_file, "</log>\n");
		fclose (l->log_file);
	}
	free (l);
}


typedef struct {
	logger *l;
	char *path;
	char *name;
	char *artist;
	char *album;
	char *length;
} logger_data;



static logger_data *
logger_data_new (logger *l,
		 const char *url,
		 const char *username,
		 const char *password,
		 const char *path)
{
	logger_data *d = malloc (sizeof(logger_data));

	d->l = l;
	if (path)
		d->path = strdup (path);
	else
		d->path = NULL;
	d->name = NULL;
	d->artist = NULL;
	d->album = NULL;
	d->length = NULL;
	return d;
}


static void
logger_data_destroy (logger_data *d)
{
	if (!d)
		return;
	if (d->path)
		free (d->path);
	if (d->name)
		free (d->name);
	if (d->artist)
		free (d->artist);
	if (d->album)
		free (d->album);
	if (d->length)
		free (d->length);
	free (d);
}


static void
logger_data_complete (logger_data *d)
{
	FileInfo *info;
	int length;
	char length_string[30];

	if (!d)
		return;

	info = file_info_new (d->path, 0, 0);
	if (!info)
		return;
	if (info->name)
		d->name = strdup (info->name);
	if (info->artist)
		d->artist = strdup (info->artist);
	if (info->album)
		d->album = strdup (info->album);
	length = (int) (info->length);

	sprintf (length_string, "%d", length);
	d->length = strdup (length_string);
	file_info_free (info);
}


static void
logger_data_log_entry (logger_data *d)
{	

	/* Log this entry to the log file */

	if (d->l->log_file) {
		time_t cur_time = time (NULL);

		fprintf (d->l->log_file, "\t<entry>\n");
		fprintf (d->l->log_file, "\t\t<time>%ld</time>\n", cur_time);
		fprintf (d->l->log_file, "\t\t<title>%s</title>\n", d->name);
		fprintf (d->l->log_file, "\t\t<artist>%s</artist>\n", d->artist);
		fprintf (d->l->log_file, "\t\t<album>%s</album>\n", d->album);
		fprintf (d->l->log_file, "\t\t<seconds>%s</seconds>\n", d->length);
		fprintf (d->l->log_file, "\t</entry>\n");
	}
}


static void
live365_log_file (logger_data *d)
{
	CURL *url;
	struct HttpPost *post = NULL;
	struct HttpPost *end = NULL;
	char *filename = NULL;

        /* Create mock file name for "least popular tracks" feature */

	asprintf (&filename, "%s - %s - %s", d->name, d->artist, d->album);
	
	url = curl_easy_init ();
	curl_easy_setopt (url, CURLOPT_URL,
			  "http://asong.live365.com/cgi-bin/add_song.cgi");
	
	curl_easy_setopt (url, CURLOPT_NOSIGNAL, 1);

        /* Setup post data */

	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "handle",
		      CURLFORM_COPYCONTENTS, d->l->username,
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "pass",
		      CURLFORM_COPYCONTENTS, d->l->password,
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "version",
		      CURLFORM_COPYCONTENTS, "2",
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "title",
		      CURLFORM_COPYCONTENTS, d->name,
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "artist",
		      CURLFORM_COPYCONTENTS, d->artist,
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "album",
		      CURLFORM_COPYCONTENTS, d->album,
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "seconds",
		      CURLFORM_COPYCONTENTS, d->length,
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "fileName",
		      CURLFORM_COPYCONTENTS, filename,
		      CURLFORM_END);
	curl_easy_setopt (url, CURLOPT_HTTPPOST, post);
	curl_easy_perform (url);
	curl_easy_cleanup (url);
	curl_formfree (post);
	logger_data_destroy (d);
	if (filename)
		free (filename);
}


static void
shoutcast_log_file (logger_data *d)
{
	CURL *url;
	struct HttpPost *post = NULL;
	struct HttpPost *end = NULL;
	char *filename = NULL;
	char *address;
	CURLcode ret;
	char *encoded_password;
	char *encoded_filename;
	


        /* Create mock file name for "least popular tracks" feature */

	asprintf (&filename, "%s - %s - %s", d->artist, d->name, d->album);
	encoded_filename = curl_escape (filename, strlen(filename));
	encoded_password = curl_escape (d->l->password, strlen(d->l->password));
	
	asprintf (&address, "%s/admin.cgi?mode=updinfo&pass=%s&song=%s", d->l->url, encoded_password, encoded_filename);
	url = curl_easy_init ();
	curl_easy_setopt (url, CURLOPT_URL,
			  address);
	
	curl_easy_setopt (url, CURLOPT_NOSIGNAL, 1);

	curl_easy_setopt (url, CURLOPT_USERAGENT, "Mozilla");
	ret = curl_easy_perform (url);
	curl_easy_cleanup (url);
	logger_data_destroy (d);
	if (address)
		free (address);
	
	if (filename) {
		free (filename);
		curl_free (encoded_filename);
	}
	if (encoded_password)
		curl_free (encoded_password);
}


static void *
logger_log_file_thread (void *data)
{
	logger_data *d = (logger_data *) data;

	if (!d)
		return NULL;
	logger_data_complete (d);
	logger_data_log_entry (d);
	switch (d->l->type) {
	case LOGGER_TYPE_LIVE365:
		live365_log_file (d);
		break;
	case LOGGER_TYPE_SHOUTCAST:
		shoutcast_log_file (d);
		break;
	}
}


void
logger_log_file (logger *l,
		 const char *path)
{
	logger_data *d;
	pthread_t id;
	
	if (!l)
		return;
	d = logger_data_new (l, l->url, l->username, l->password, path);

	pthread_create (&id, NULL, logger_log_file_thread, d);
	pthread_detach (id);
	return;
}
