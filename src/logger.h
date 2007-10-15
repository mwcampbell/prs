/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */
#ifndef _LOGGER_H
#define _LOGGER_H

#include "db.h"


typedef enum {
	LOGGER_TYPE_LIVE365,
	LOGGER_TYPE_SHOUTCAST,
	LOGGER_TYPE_ICECAST
} LOGGER_TYPE;


typedef struct {
	LOGGER_TYPE type;
	FILE *log_file;
	char *url;
	char *username;
	char *password;
	char *mount;
} logger;


logger *
logger_new (const LOGGER_TYPE type,
	    const char *log_file_name,
	    const char *url,
	    const char *username,
	    const char *password,
	    const char *mount);
void
logger_destroy (logger *l);
void
logger_log_file (logger *l,
		 const char *path);


#endif
