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
 * soundcard.h: Miscelaneous soundcard functions.
 *
 */

int
soundcard_get_fd (void);
void
soundcard_set_fd (int fd);
int
soundcard_get_rate (void);
void
soundcard_set_rate (int rate);
int
soundcard_get_channels (void);
void
soundcard_set_channels (int channels);
int
soundcard_get_duplex (void);
void
soundcard_set_duplex (int duplex);
int
soundcard_setup (int rate,
		 int channels,
		 int latency);
