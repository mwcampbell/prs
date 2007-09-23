/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
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
prs_list_get_item (list *l, int item);
void
prs_list_free (list *l);
list *
prs_list_append (list *l,
	     void *data);
list *
prs_list_prepend (list *l,
	      void *data);
list *
prs_list_insert_before (list *l,
		    void *data);
list *
prs_list_insert_after (list *l,
		    void *data);
list *
prs_list_delete_item (list *l,
		  list *item);
int
prs_list_length (list *l);
list *
prs_list_copy (list *l);
list *
prs_list_reverse (list *l);



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
