/*
 *
 * list.h
 *
 * Linked list code for prs
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
#endif
