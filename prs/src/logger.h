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
 * logger.h: Header for the logger object.
 *
 */
#ifndef _LOGGER_H
#define _LOGGER_H

#include "db.h"


typedef enum {
	LOGGER_TYPE_LIVE365,
	LOGGER_TYPE_SHOUTCAST
} LOGGER_TYPE;


typedef struct {
	LOGGER_TYPE type;
	FILE *log_file;
	char *url;
	char *username;
	char *password;
} logger;


logger *
logger_new (const LOGGER_TYPE type,
	    const char *log_file_name,
	    const char *url,
	    const char *username,
	    const char *password);
void
logger_destroy (logger *l);
void
logger_log_file (logger *l,
		 const char *path);


#endif
