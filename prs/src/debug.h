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
 * debug.h: Header file for PRS debugging functions.
 *
 */

#ifndef _DEBUG_H
#define _DEBUG_H_



#define DEBUG_FLAGS_AUTOMATION 0x01
#define DEBUG_FLAGS_SCHEDULER 0x02
#define DEBUG_FLAGS_DATABASE 0x04
#define DEBUG_FLAGS_TELNET 0x08
#define DEBUG_FLAGS_GENERAL 0x10
#define DEBUG_FLAGS_MIXER 0x20
#define DEBUG_FLAGS_FILE_INFO 0x40
#define DEBUG_FLAGS_CODEC 0x80
#define DEBUG_FLAGS_ALL 0xff

void
debug_set_flags (int debug_flags);
void
debug_printf (int debug_flags,
	      const char *format,
	      ...);



#endif
