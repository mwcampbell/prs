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
 * list.h: header file for the implementation of a linked list.
 *
 */

#ifndef _PRS_LIST_H_
#define _PRS_LIST_H_



typedef struct _list list;
struct _list {
  list *next;
  list *prev;
  void *data;
};



/*
 *
 * General list manipulation
 *
 */



void *
list_get_item (list *l, int item);
void
list_free (list *l);
list *
list_append (list *l,
	     void *data);
list *
list_prepend (list *l,
	      void *data);
list *
list_insert_before (list *l,
		    void *data);
list *
list_insert_after (list *l,
		    void *data);
list *
list_delete_item (list *l,
		  list *item);
int
list_length (list *l);
list *
list_copy (list *l);
list *
list_reverse (list *l);



/*
 *
 * String list convenience functions
 *
 */



void
string_list_free (list *l);
list *
string_list_prepend (list *l,
		    const char *s);
list *
string_list_append (list *l,
		    const char *s);
char **
string_list_to_array (list *l);
#endif
