/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
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
#include "alsamixeroutput.h"
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
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"stream")) {
			double retry_delay;

			/* create the shout connection */

			s = shout_new ();
			tmp = xmlGetProp (cur, (xmlChar*)"host");
			if (tmp) {
				shout_set_host (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"port");
			if (tmp) {
				shout_set_port (s, atoi((char*)tmp));
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"password");
			if (tmp) {
				shout_set_password (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"mount");
			if (tmp) {
				shout_set_mount (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"title");
			if (tmp) {
				shout_set_name (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"url");
			if (tmp) {
				shout_set_url (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"genre");
			if (tmp) {
				shout_set_genre (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"user");
			if (tmp) {
				shout_set_user (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"agent");
			if (tmp) {
				shout_set_agent (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"description");
			if (tmp) {
				shout_set_description (s, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"bitrate");
			if (tmp) {
				shout_set_audio_info (s, SHOUT_AI_BITRATE, (char*)tmp);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"protocol");
			shout_set_protocol (s, SHOUT_PROTOCOL_XAUDIOCAST);
			if (tmp) {
				if (!xmlStrcasecmp (tmp, (xmlChar*)"xaudiocast"))
					shout_set_protocol (s, SHOUT_PROTOCOL_XAUDIOCAST);
				else if (!xmlStrcasecmp (tmp, (xmlChar*)"http"))
					shout_set_protocol (s, SHOUT_PROTOCOL_HTTP);
				if (!xmlStrcasecmp (tmp, (xmlChar*)"icy"))
					shout_set_protocol (s, SHOUT_PROTOCOL_ICY);
				xmlFree (tmp);
			}
			tmp = xmlGetProp (cur, (xmlChar*)"format");
			if (!xmlStrcasecmp (tmp, (xmlChar*)"mp3"))
				shout_set_format (s, SHOUT_FORMAT_MP3);
			if (!xmlStrcasecmp (tmp, (xmlChar*)"vorbis"))
				shout_set_format (s, SHOUT_FORMAT_VORBIS);
			else if (tmp)
				xmlFree (tmp);
			tmp = xmlGetProp (cur, (xmlChar*)"stereo");
			if (tmp) {
				stereo = atoi ((char*)tmp);
				xmlFree (tmp);
			}
			else
				stereo = 1;
			tmp = xmlGetProp (cur, (xmlChar*)"rate");
			if (tmp) {
				rate = atoi ((char*)tmp);
				xmlFree (tmp);
			}
			else
				rate = 44100;
			tmp = xmlGetProp (cur, (xmlChar*)"channels");
			if (tmp) {
				channels = atoi ((char*)tmp);
				xmlFree (tmp);
			}
			else
				channels = 2;
			tmp = xmlGetProp (cur, (xmlChar*)"retry_delay");
			if (tmp) {
				retry_delay = atof ((char*)tmp);
				xmlFree (tmp);
			}
			else
				retry_delay = 10.0;
			archive_file_name = xmlGetProp (cur, (xmlChar*)"archive_file_name");
			name = xmlGetProp (cur, (xmlChar*)"name");

			/* Process encoder args */

			child = cur->xmlChildrenNode;
			while (child) {
				if (!xmlStrcasecmp (child->name, (xmlChar*)"encoder_arg")) {
					child = child->xmlChildrenNode;
					fprintf (stderr, "Adding encoder arg %s.\n", (char*)child->content);
					args = string_list_prepend (args, (char*)child->content);
					child = child->parent;
				}
				child = child->next;
			}
			o = shout_mixer_output_new ((char*)name, rate, channels,
						    m->latency, s, stereo, args, (char*)archive_file_name, retry_delay);
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
	float threshhold, ratio, attack_time, release_time, output_gain;

	threshhold = atof ((char*)xmlGetProp (cur, (xmlChar*)"threshhold"));
	ratio = atof ((char*)xmlGetProp (cur, (xmlChar*)"ratio"));
	attack_time = atof ((char*)xmlGetProp (cur, (xmlChar*)"attack_time"));
	release_time = atof ((char*)xmlGetProp (cur, (xmlChar*)"release_time"));
	output_gain = atof ((char*)xmlGetProp (cur, (xmlChar*)"output_gain"));
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
	float freq;
	float threshhold;
	float ratio;
	float attack_time;
	float release_time;
	float pre_process_gain;
	float output_gain;
	float link;

	f = multiband_audio_compressor_new (b->rate,
					    b->channels,
					    m->latency);

	/* Add bands */

	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"band")) {
			freq = atof ((char*)xmlGetProp(cur, (xmlChar*)"freq"));
			threshhold = atof ((char*)xmlGetProp(cur, (xmlChar*)"threshhold"));
			ratio = atof ((char*)xmlGetProp(cur, (xmlChar*)"ratio"));
			attack_time = atof ((char*)xmlGetProp(cur, (xmlChar*)"attack_time"));
			release_time = atof ((char*)xmlGetProp(cur, (xmlChar*)"release_time"));
			pre_process_gain = atof ((char*)xmlGetProp(cur, (xmlChar*)"pre_process_gain"));
			output_gain = atof ((char*)xmlGetProp(cur, (xmlChar*)"output_gain"));
			link = atof ((char*)xmlGetProp (cur, (xmlChar*)"link"));
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
	xmlChar *sc_name;
	xmlChar *type;
	xmlChar *rate;
	xmlChar *channels;
	
	name = xmlGetProp (cur, (xmlChar*)"name");
	sc_name = xmlGetProp (cur, (xmlChar*)"sc_name");
	type = xmlGetProp (cur, (xmlChar*)"type");
	rate = xmlGetProp (cur, (xmlChar*)"rate");
	channels = xmlGetProp (cur, (xmlChar*)"channels");
	if (!xmlStrcasecmp (type, (xmlChar*)"oss"))
		o = oss_mixer_output_new ((char*)sc_name,
					  (char*)name,
					  atoi((char*)rate),
					  atoi((char*)channels),
					  m->latency);
	else if (!xmlStrcasecmp (type, (xmlChar*)"alsa"))
		o = alsa_mixer_output_new ((char*)sc_name,
					   (char*)name,
					   atoi((char*)rate),
					   atoi((char*)channels),
					   m->latency);
	else if (!xmlStrcasecmp (type, (xmlChar*)"wave"))
		o = file_mixer_output_new ((char*)name,
					   atoi((char*)rate),
					   atoi((char*)channels),
					   m->latency);
	if (o) {
		mixer_add_output (m, o);   
	}
	if (name)
		xmlFree (name);
	if (sc_name)
		xmlFree (sc_name);
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
	xmlChar *sc_name;
	xmlChar *rate;
	xmlChar *channels;
	xmlChar *name;
	xmlChar *location;
	xmlChar *type;

	name = xmlGetProp (cur, (xmlChar*)"name");
	type = xmlGetProp (cur, (xmlChar*)"type");
	sc_name = xmlGetProp (cur, (xmlChar*)"sc_name");
	rate = xmlGetProp (cur, (xmlChar*)"rate");
	channels = xmlGetProp (cur, (xmlChar*)"channels");
	if (!xmlStrcasecmp (type, (xmlChar*)"oss"))
		ch = oss_mixer_channel_new ((char*)sc_name,
					    (char*)name,
					    atoi ((char*)rate),
					    atoi((char*)channels),
					    m->latency);
	if (ch) {
		ch->enabled = 0;
		mixer_add_channel (m, ch);   
	}
	if (name)
		xmlFree (name);
	if (sc_name)
		xmlFree (sc_name);
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
	xmlChar *channel_name = xmlGetProp (cur, (xmlChar*)"channel");
	xmlChar *bus_name = xmlGetProp (cur, (xmlChar*)"bus");
	xmlChar *output_name = xmlGetProp (cur, (xmlChar*)"output");
	if (channel_name && bus_name)
		mixer_patch_channel (m, (char*)channel_name, (char*)bus_name);
	else if (bus_name && output_name)
		mixer_patch_bus (m, (char*)bus_name, (char*)output_name);
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

	bus_name = xmlGetProp (cur, (xmlChar*)"name");
	rate = xmlGetProp (cur, (xmlChar*)"rate");
	channels = xmlGetProp (cur, (xmlChar*)"channels");
	b = mixer_bus_new ((char*)bus_name, atoi((char*)rate), atoi((char*)channels), m->latency);
	if (bus_name)
		xmlFree (bus_name);
	if (rate)
		xmlFree (rate);
	if (channels)
		xmlFree (channels);
		

	/* Process filters and output patches */

	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"audiocompressor"))
			audio_compressor_config (m, b, cur);
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"multibandaudiocompressor"))
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
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"bus"))
			mixer_bus_config (m, cur);
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"output"))
			mixer_output_config (m, cur);
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"channel"))
			mixer_channel_config (m, cur);
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"patch"))
			mixer_patch_config (m, cur);
		cur = cur->next;
	}
}



static void
telnet_config (PRS *prs, xmlNodePtr cur)
{
	xmlChar *password = xmlGetProp (cur, (xmlChar*)"password");
	xmlChar *port = xmlGetProp (cur, (xmlChar*)"port");
	if (password != NULL) {
		prs->password = strdup ((char*)password);
		xmlFree (password);
	}
	if (port == NULL)
		prs->telnet_port = 4777;
	else {
		prs->telnet_port = atoi ((char*)port);
		xmlFree (port);
	}
	prs->telnet_interface = 1;
}



static logger *
logger_config (xmlNodePtr cur)
{
	logger *l = NULL;
	LOGGER_TYPE type;
	xmlChar *url = xmlGetProp (cur, (xmlChar*)"url");
	xmlChar *username = xmlGetProp (cur, (xmlChar*)"username");
	xmlChar *password = xmlGetProp (cur, (xmlChar*)"password");
	xmlChar *mount = xmlGetProp (cur, (xmlChar*)"mount");
	xmlChar *type_string = xmlGetProp (cur, (xmlChar*)"type");
	xmlChar *log_file_name = xmlGetProp (cur, (xmlChar*)"log_file_name");
	
	if (!xmlStrcasecmp (type_string, (xmlChar*)"live365")) {
		l = logger_new (LOGGER_TYPE_LIVE365,
				(char*)log_file_name, NULL,
				   (char*)username, (char*)password, NULL);
	}
	if (!xmlStrcasecmp (type_string, (xmlChar*)"shoutcast")) {
		l = logger_new (LOGGER_TYPE_SHOUTCAST,
				(char*)log_file_name, (char*)url,
				   (char*)username, (char*)password, NULL);
	}
	if (!xmlStrcasecmp (type_string, (xmlChar*)"icecast")) {
		l = logger_new (LOGGER_TYPE_ICECAST,
				(char*)log_file_name, (char*)url,
				   (char*)username, (char*)password, (char*)mount);
	}
	if (url)
		xmlFree (url);
	if (username)
		xmlFree (username);
	if (password)
		xmlFree (password);
	if (mount)
		xmlFree (mount);
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
		return -1;
	}
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcasecmp (cur->name, (xmlChar*)"stream_config"))
			stream_config (prs->mixer, cur);
		else if (!xmlStrcasecmp (cur->name, (xmlChar*)"mixer_config")) 
			mixer_config (prs->mixer, cur);
		else if (!xmlStrcasecmp (cur->name, (xmlChar*)"telnet"))
			telnet_config (prs, cur);
		else if (!xmlStrcasecmp (cur->name, (xmlChar*)"db"))
			db_config (prs->db, cur);
		else if (!xmlStrcasecmp (cur->name, (xmlChar*)"logger")) {
			prs->logger = logger_config (cur);
			mixer_automation_add_logger (prs->automation,
						     prs->logger);
		}
		cur = cur->next;
	}  
	xmlFreeDoc (doc);
	return 0;
}
  
