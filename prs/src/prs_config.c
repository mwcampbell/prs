#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "prs_config.h"
#include "audiocompressor.h"
#include "multibandaudiocompressor.h"
#include "ossmixeroutput.h"
#include "shoutmixeroutput.h"




void
stream_config (mixer *m, xmlNodePtr cur)
{
	MixerOutput *o;
	shout_t *s;
	xmlChar  *tmp;
	int stereo;
	
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if (!xmlStrcmp (cur->name, "stream")) {
			
			/* create the shout connection */

			s = shout_new ();
			tmp = xmlGetProp (cur, "host");
			if (tmp)
				shout_set_host (s, tmp);
			tmp = xmlGetProp (cur, "port");
			if (tmp)
				shout_set_port (s, atoi(tmp));
			tmp = xmlGetProp (cur, "password");
			if (tmp)
				shout_set_password (s, tmp);
			printf ("Set password to %s.\n", tmp);
			tmp = xmlGetProp (cur, "mount");
			if (tmp)
				shout_set_mount (s, tmp);
			tmp = xmlGetProp (cur, "title");
			if (tmp)
				shout_set_name (s, tmp);
			tmp = xmlGetProp (cur, "url");
			if (tmp)
				shout_set_url (s, tmp);
			tmp = xmlGetProp (cur, "genre");
			if (tmp)
				shout_set_genre (s, tmp);
			tmp = xmlGetProp (cur, "user");
			if (tmp)
				shout_set_user (s, tmp);
			tmp = xmlGetProp (cur, "agent");
			if (tmp)
				shout_set_agent (s, tmp);
			tmp = xmlGetProp (cur, "description");
			if (tmp)
				shout_set_description (s, tmp);
			tmp = xmlGetProp (cur, "bitrate");
			if (tmp)
				shout_set_bitrate (s, atoi (tmp));
			tmp = xmlGetProp (cur, "protocol");
			shout_set_protocol (s, SHOUT_PROTOCOL_XAUDIOCAST);
			if (tmp) {
				if (!xmlStrcmp (tmp, "xaudiocast"))
					shout_set_protocol (s, SHOUT_PROTOCOL_XAUDIOCAST);
				else if (!xmlStrcmp (tmp, "http"))
					shout_set_protocol (s, SHOUT_PROTOCOL_HTTP);
				if (!xmlStrcmp (tmp, "icy"))
					shout_set_protocol (s, SHOUT_PROTOCOL_ICY);
				if (!xmlStrcmp (tmp, "ice"))
					shout_set_protocol (s, SHOUT_PROTOCOL_ICE);
				}
			tmp = xmlGetProp (cur, "format");
			if (!xmlStrcmp (tmp, "mp3"))
				shout_set_format (s, SHOUT_FORMAT_MP3);
			if (!xmlStrcmp (tmp, "vorbis"))
				shout_set_format (s, SHOUT_FORMAT_VORBIS);
			tmp = xmlGetProp (cur, "stereo");
			if (tmp)
				stereo = atoi (tmp);
			else
				stereo = 1;
			tmp = xmlGetProp (cur, "name");
			o = shout_mixer_output_new (tmp, 44100, 2, m->latency, s, stereo);
			mixer_add_output (m, o);
		}
		cur = cur->next;
	}
	fprintf (stderr, "Processing stream configuration...\n");
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
	fprintf (stderr, "Compressor: %lf, %lf, %lf, %lf, %lf.\n", threshhold, ratio, attack_time, release_time, output_gain);
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
			fprintf (stderr, "Adding %lf.\n", freq);
			multiband_audio_compressor_add_band (f,
							     freq,
							     threshhold,
							     ratio,
							     attack_time,
							     release_time,
							     pre_process_gain,
							     output_gain);
		}
		cur = cur->next;
	}
	mixer_bus_add_filter (b, f);
}



static void
mixer_output_config (mixer *m, xmlNodePtr cur)
{
	MixerOutput *o = NULL;
	int rate, channels;
	xmlChar *name;
	xmlChar *type;

	name = xmlGetProp (cur, "name");
	type = xmlGetProp (cur, "type");
	rate = atoi (xmlGetProp (cur, "rate"));
	channels = atoi (xmlGetProp (cur, "channels"));
	if (!xmlStrcmp (type, "oss"))
		o = oss_mixer_output_new (name, rate, channels, m->latency);
	if (o) {
		mixer_add_output (m, o);   
	}
}



static void
mixer_patch_config (mixer *m, xmlNodePtr cur)
{
	xmlChar *bus_name = xmlGetProp (cur, "bus");
	xmlChar *output_name = xmlGetProp (cur, "output");
	mixer_patch_bus (m, bus_name, output_name);
}



static void
mixer_bus_config (mixer *m, xmlNodePtr cur)
{
	MixerBus *b;
	int rate, channels;
	xmlChar *bus_name;

	bus_name = xmlGetProp (cur, "name");
	rate = atoi (xmlGetProp (cur, "rate"));
	channels = atoi (xmlGetProp (cur, "channels"));
	fprintf (stderr, "Creating mixer bus %s: %d, %d.\n", bus_name, rate, channels);
	b = mixer_bus_new (bus_name, rate, channels, m->latency);


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
	fprintf (stderr, "Processing mixer configuration...\n");
	while (cur != NULL) {
		if (!xmlStrcmp (cur->name, "bus"))
			mixer_bus_config (m, cur);
		if (!xmlStrcmp (cur->name, "output"))
			mixer_output_config (m, cur);
		if (!xmlStrcmp (cur->name, "patch"))
			mixer_patch_config (m, cur);
		cur = cur->next;
	}
}



int
prs_config (mixer *m)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile ("prs.conf");
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
			stream_config (m, cur);
		else if (!xmlStrcmp (cur->name, "mixer_config")) 
			mixer_config (m, cur);
		else
			fprintf (stderr, "Skipping %s node...\n", cur->name);
		cur = cur->next;
	}  
	xmlFreeDoc (doc);
}
  
