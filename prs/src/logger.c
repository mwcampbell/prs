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
 * logger.c: Implementation of logger object.
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
	    const char *url,
	    const char *username,
	    const char *password)
{
	logger *l = malloc (sizeof(logger));

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
	free (l);
}


typedef struct {
	char *url;
	char *username;
	char *password;
	char *path;
	char *name;
	char *artist;
	char *album;
	char *length;
} logger_data;



static logger_data *
logger_data_new (const char *url,
		 const char *username,
		 const char *password,
		 const char *path)
{
	logger_data *d = malloc (sizeof(logger_data));

	if (url)
		d->url = strdup (url);
	else
		d->url = NULL;
	if (username)
		d->username = strdup (username);
	else
		d->username = NULL;
	if (password)
		d->password = strdup (password);
	else
		d->password = NULL;
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
	if (d->url)
		free (d->url);
	if (d->username)
		free (d->username);
	if (d->password)
		free (d->password);
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
	length = (int) (info->audio_out-info->audio_in);

	sprintf (length_string, "%d", length);
	d->length = strdup (length_string);
	file_info_free (info);
}


static void *
live365_log_file (void *data)
{
	logger_data *d = (logger_data *) data;
	CURL *url;
	struct HttpPost *post = NULL;
	struct HttpPost *end = NULL;
	
	logger_data_complete (d);
	url = curl_easy_init ();
	curl_easy_setopt (url, CURLOPT_URL,
			  "http://asong.live365.com/cgi-bin/add_song.cgi");
	
	curl_easy_setopt (url, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt (url, CURLOPT_TIMEOUT, 30);

        /* Setup post data */

	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "handle",
		      CURLFORM_COPYCONTENTS, d->username,
		      CURLFORM_END);
	curl_formadd (&post, &end,
		      CURLFORM_COPYNAME, "pass",
		      CURLFORM_COPYCONTENTS, d->password,
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
	curl_easy_setopt (url, CURLOPT_HTTPPOST, post);
	curl_easy_perform (url);
	curl_formfree (post);
	curl_easy_cleanup (url);
	logger_data_destroy (d);
	pthread_exit (NULL);
}


void
logger_log_file (logger *l,
		 const char *path)
{
	logger_data *d;
	pthread_t id;
	
	if (!l)
		return;
	d = logger_data_new (l->url, l->username, l->password, path);

	switch (l->type) {
	case LOGGER_TYPE_LIVE365:
		pthread_create (&id, NULL, live365_log_file, d);
		break;
	}
	return;
}
