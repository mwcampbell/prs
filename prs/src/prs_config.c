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
 * prs_config.c: PRS configuration file processing.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "prs_config.h"
#include "db.h"
#include "audiocompressor.h"
#include "multibandaudiocompressor.h"
#include "ossmixeroutput.h"
#include "ossmixerchannel.h"
#include "shoutmixeroutput.h"
#include "filemixeroutput.h"
#include "logger.h"




static void
stream_config (mixer *m, xmlNodePtr cur)
{
	MixerOutput *o;
	shout_t *s;
	xmlNodePtr child;
	xmlChar *tmp;
	xmlChar *name;
	int stereo;
	int rate, channels;
	list *args = NULL;
	xmlChar *archive_file_name;
	
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if (!xmlStrcmp (cur->name, "stream")) {
			double retry_delay;

			/* create the shout connection */

			s = shout_new ();
			tmp = xmlGetProp (cur, "host");
			if (tmp) {
				shout_set_host (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "port");
			if (tmp) {
				shout_set_port (s, atoi(tmp));
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "password");
			if (tmp) {
				shout_set_password (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "mount");
			if (tmp) {
				shout_set_mount (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "title");
			if (tmp) {
				shout_set_name (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "url");
			if (tmp) {
				shout_set_url (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "genre");
			if (tmp) {
				shout_set_genre (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "user");
			if (tmp) {
				shout_set_user (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "agent");
			if (tmp) {
				shout_set_agent (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "description");
			if (tmp) {
				shout_set_description (s, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "bitrate");
			if (tmp) {
				shout_set_audio_info (s, SHOUT_AI_BITRATE, tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "protocol");
			shout_set_protocol (s, SHOUT_PROTOCOL_XAUDIOCAST);
			if (tmp) {
				if (!xmlStrcmp (tmp, "xaudiocast"))
					shout_set_protocol (s, SHOUT_PROTOCOL_XAUDIOCAST);
				else if (!xmlStrcmp (tmp, "http"))
					shout_set_protocol (s, SHOUT_PROTOCOL_HTTP);
				if (!xmlStrcmp (tmp, "icy"))
					shout_set_protocol (s, SHOUT_PROTOCOL_ICY);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, "format");
			if (!xmlStrcmp (tmp, "mp3"))
				shout_set_format (s, SHOUT_FORMAT_MP3);
			if (!xmlStrcmp (tmp, "vorbis"))
				shout_set_format (s, SHOUT_FORMAT_VORBIS);
			else if (tmp)
				xmlFree (tmp);
			tmp = xmlGetProp (cur, "stereo");
			if (tmp) {
				stereo = atoi (tmp);
				xmlFree (tmp);
			}
			else
				stereo = 1;
			tmp = xmlGetProp (cur, "rate");
			if (tmp) {
				rate = atoi (tmp);
				xmlFree (tmp);
			}
			else
				rate = 44100;
			tmp = xmlGetProp (cur, "channels");
			if (tmp) {
				channels = atoi (tmp);
				xmlFree (tmp);
			}
			else
				channels = 2;
			tmp = xmlGetProp (cur, "retry_delay");
			if (tmp) {
				retry_delay = atof (tmp);
				xmlFree (tmp);
			}
			else
				retry_delay = 10.0;
			archive_file_name = xmlGetProp (cur, "archive_file_name");
			name = xmlGetProp (cur, "name");

			/* Process encoder args */

			child = cur->xmlChildrenNode;
			while (child) {
				if (!xmlStrcmp (child->name, "encoder_arg")) {
					child = child->xmlChildrenNode;
					fprintf (stderr, "Adding encoder arg %s.\n", child->content);
					args = string_list_prepend (args, child->content);
					child = child->parent;
				}
				child = child->next;
			}
			o = shout_mixer_output_new (name, rate, channels,
						    m->latency, s, stereo, args, archive_file_name, retry_delay);
			mixer_add_output (m, o);
		if (name)
			xmlFree (name);
		if (archive_file_name)
			xmlFree (archive_file_name);
		}
		cur = cur->next;
		args = NULL;
	}
}



static void
audio_compressor_config (mixer *m, MixerBus *b, xmlNodePtr cur)
{
	AudioFilter *f;
	double threshhold, ratio, attack_time, release_time, output_gain;

	threshhold = atof (xmlGetProp (cur, "threshhold"));
	ratio = atof (xmlGetProp (cur, "ratio"));
	attack_time = atof (xmlGetProp (cur, "attack_time"));
	release_time = atof (xmlGetProp (cur, "release_time"));
	output_gain = atof (xmlGetProp (cur, "output_gain"));
	f = audio_compressor_new (b->rate,
				  b->channels,
				  m->latency,
				  threshhold,
				  ratio,
				  attack_time,
				  release_time,
				  output_gain);
	mixer_bus_add_filter (b, f);
}



static void
multiband_audio_compressor_config (mixer *m, MixerBus *b, xmlNodePtr cur)
{
	AudioFilter *f;
	double freq;
	double threshhold;
	double ratio;
	double attack_time;
	double release_time;
	double pre_process_gain;
	double output_gain;
	double link;

	f = multiband_audio_compressor_new (b->rate,
					    b->channels,
					    m->latency);

	/* Add bands */

	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp (cur->name, "band")) {
			freq = atof (xmlGetProp(cur, "freq"));
			threshhold = atof (xmlGetProp(cur, "threshhold"));
			ratio = atof (xmlGetProp(cur, "ratio"));
			attack_time = atof (xmlGetProp(cur, "attack_time"));
			release_time = atof (xmlGetProp(cur, "release_time"));
			pre_process_gain = atof (xmlGetProp(cur, "pre_process_gain"));
			output_gain = atof (xmlGetProp(cur, "output_gain"));
			link = atof (xmlGetProp (cur, "link"));
			if (freq >= (b->rate/2)*.9)
				freq = (b->rate/2)*.9;
			multiband_audio_compressor_add_band (f,
							     freq,
							     threshhold,
							     ratio,
							     attack_time,
							     release_time,
							     pre_process_gain,
							     output_gain,
							     link);
		}
		cur = cur->next;
	}
	mixer_bus_add_filter (b, f);
}



static void
mixer_output_config (mixer *m, xmlNodePtr cur)
{
	MixerOutput *o = NULL;
	xmlChar *name;
	xmlChar *type;
	xmlChar *rate;
	xmlChar *channels;
	
	name = xmlGetProp (cur, "name");
	type = xmlGetProp (cur, "type");
	rate = xmlGetProp (cur, "rate");
	channels = xmlGetProp (cur, "channels");
	if (!xmlStrcmp (type, "oss"))
		o = oss_mixer_output_new (name,
					  atoi(rate),
					  atoi(channels),
					  m->latency);
	else if (!xmlStrcmp (type, "wave"))
		o = file_mixer_output_new (name,
					   atoi(rate),
					   atoi(channels),
					   m->latency);
	if (o) {
		mixer_add_output (m, o);   
	}
	if (name)
		xmlFree (name);
	if (type)
		xmlFree (type);
	if (rate)
		xmlFree (rate);
	if (channels)
		xmlFree (channels);
}



static void
mixer_channel_config (mixer *m, xmlNodePtr cur)
{
	MixerChannel *ch = NULL;
	xmlChar *rate;
	xmlChar *channels;
	xmlChar *name;
	xmlChar *location;
	xmlChar *type;

	name = xmlGetProp (cur, "name");
	type = xmlGetProp (cur, "type");
	rate = xmlGetProp (cur, "rate");
	channels = xmlGetProp (cur, "channels");
	if (!xmlStrcmp (type, "oss"))
		ch = oss_mixer_channel_new (name,atoi( rate), atoi(channels),
					    m->latency);
	if (ch) {
		ch->enabled = 0;
		mixer_add_channel (m, ch);   
	}
	if (name)
		xmlFree (name);
	if (location)
		xmlFree (location);
	if (rate)
		xmlFree (rate);
	if (channels)
		xmlFree (channels);
}



static void
mixer_patch_config (mixer *m, xmlNodePtr cur)
{
	xmlChar *channel_name = xmlGetProp (cur, "channel");
	xmlChar *bus_name = xmlGetProp (cur, "bus");
	xmlChar *output_name = xmlGetProp (cur, "output");
	if (channel_name && bus_name)
		mixer_patch_channel (m, channel_name, bus_name);
	else if (bus_name && output_name)
		mixer_patch_bus (m, bus_name, output_name);
	if (channel_name)
		xmlFree (channel_name);
	if (bus_name)
		xmlFree (bus_name);
	if (output_name)
		xmlFree (output_name);
}



static void
mixer_bus_config (mixer *m, xmlNodePtr cur)
{
	MixerBus *b;
	xmlChar *rate;
	xmlChar *channels;
	xmlChar *bus_name;

	bus_name = xmlGetProp (cur, "name");
	rate = xmlGetProp (cur, "rate");
	channels = xmlGetProp (cur, "channels");
	b = mixer_bus_new (bus_name, atoi(rate), atoi(channels), m->latency);
	if (bus_name)
		xmlFree (bus_name);
	if (rate)
		xmlFree (rate);
	if (channels)
		xmlFree (channels);
		

	/* Process filters and output patches */

	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp (cur->name, "audiocompressor"))
			audio_compressor_config (m, b, cur);
		if (!xmlStrcmp (cur->name, "multibandaudiocompressor"))
			multiband_audio_compressor_config (m, b, cur);
		cur = cur->next;
	}
	mixer_add_bus (m, b);
}



static void
mixer_config (mixer *m, xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp (cur->name, "bus"))
			mixer_bus_config (m, cur);
		if (!xmlStrcmp (cur->name, "output"))
			mixer_output_config (m, cur);
		if (!xmlStrcmp (cur->name, "channel"))
			mixer_channel_config (m, cur);
		if (!xmlStrcmp (cur->name, "patch"))
			mixer_patch_config (m, cur);
		cur = cur->next;
	}
}



static void
telnet_config (PRS *prs, xmlNodePtr cur)
{
	xmlChar *password = xmlGetProp (cur, "password");
	xmlChar *port = xmlGetProp (cur, "port");
	if (password != NULL) {
		prs->password = strdup (password);
		xmlFree (password);
	}
	if (port == NULL)
		prs->telnet_port = 4777;
	else {
		prs->telnet_port = atoi (port);
		xmlFree (port);
	}
	prs->telnet_interface = 1;
}



static logger *
logger_config (xmlNodePtr cur)
{
	logger *l = NULL;
	LOGGER_TYPE type;
	xmlChar *username = xmlGetProp (cur, "username");
	xmlChar *password = xmlGetProp (cur, "password");
	xmlChar *type_string = xmlGetProp (cur, "type");
	xmlChar *log_file_name = xmlGetProp (cur, "log_file_name");
	
	if (!xmlStrcmp (type_string, "live365")) {
		l = logger_new (LOGGER_TYPE_LIVE365,
				log_file_name, NULL,
				   username, password);
	}
	if (username)
		xmlFree (username);
	if (password)
		xmlFree (password);
	if (type_string)
		xmlFree (type_string);
	if (log_file_name)
		xmlFree (log_file_name);
	return l;
}


int
prs_config (PRS *prs, const char *filename)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile (filename);
	if (doc == NULL) {
		fprintf (stderr, "Can't process configuration file.\n");
		return -1;
	}
	cur = xmlDocGetRootElement (doc);
	if (cur == NULL) {
		fprintf (stderr, "Invalid configuration file.\n");
		xmlFreeDoc (doc);
	}
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp (cur->name, "stream_config"))
			stream_config (prs->mixer, cur);
		else if (!xmlStrcmp (cur->name, "mixer_config")) 
			mixer_config (prs->mixer, cur);
		else if (!xmlStrcmp (cur->name, "telnet"))
			telnet_config (prs, cur);
		else if (!xmlStrcmp (cur->name, "db"))
			db_config (prs->db, cur);
		else if (!xmlStrcmp (cur->name, "logger")) {
			prs->logger = logger_config (cur);
			mixer_automation_add_logger (prs->automation,
						     prs->logger);
		}
		cur = cur->next;
	}  
	xmlFreeDoc (doc);
}
  
