/*
 *
 * list.c - Linked list coe for prs
 *
 */



#include <stdio.h>
#include "list.h"



void
list_free (list *l)
{
  list *tmp = l, *next = l;

  while (tmp)
    {
      next = tmp->next;
      free (tmp);
      tmp = next;
    }
  return;
}



list *
list_append (list *l, void *data)
{
  list *tmp = l; 
  list *new_item = (list *) malloc (sizeof(list));

  while (tmp && tmp->next)
      tmp = tmp->next;

  new_item->prev = tmp;
  new_item->next = NULL;
  new_item->data = data;

  if (tmp)
    {
      tmp->next = new_item;
      return l;
    }

  else
    return new_item;
}



list *
list_prepend (list *l, void *data)
{
  list *tmp = l;
  list *new_item = (list *) malloc (sizeof(list));

  if (l)
    l->prev = new_item;
  new_item->next = l;
  new_item->prev = NULL;
  new_item->data = data;
  return new_item;
}



list *
list_insert_before (list *l,
		    void *data)
{
  list *new_item = (list *) malloc (sizeof (list));

  if (!l)
    return;
  new_item->prev = l->prev;
  if (l->prev)
    l->prev->next = new_item;
  new_item->next = l;
  l->prev = new_item;
  new_item->data = data;
  return new_item;
}



list *
list_insert_after (list *l,
		    void *data)
{
  list *new_item = (list *) malloc (sizeof (list));

  if (!l)
    return;
  new_item->prev = l;
  new_item->next = l->next;
  if (l->next)
    l->next->prev = new_item;
  l->next = new_item;
  new_item->data = data;
  return new_item;
}



list *
list_delete_item (list *l,
		  list *item)
{
  if (!l || !item)
    return NULL;
  if (item->prev)
    item->prev->next = item->next;
  else
    l = item->next;
  if (item->next)
    item->next->prev = item->prev;
  free (item);
  return l;
}



int
list_length (list *l)
{
  int i = 0;

  while (l)
  {
    i++;
    l = l->next;
  }
}



list *
list_copy (list *l)
{
  list *new_list;
  list *tmp;
  list *new_item = NULL, *prev = NULL;
  
  if (!l)
    return NULL;
  for (tmp = l; tmp; tmp = tmp->next)
    {
      new_item = (list *) malloc (sizeof(list));
      if (prev)
	prev->next = new_item;
      else
       new_list = new_item;
      new_item->prev = prev;
      new_item->next = NULL;
      new_item->data = tmp->data;
    prev = new_item;
    }
  return new_list;
}



list *
list_reverse (list *l)
{
  list *tmp;

  while (l && l->next)
    {
      tmp = l->prev;
      l->prev = l->next;
      l->next = tmp;
      l = l->prev;
    }
  return l;
}



void
string_list_free (list *l)
{
  list *tmp = l;
  while (tmp)
  {
    if (tmp->data)
      free (tmp->data);
    tmp = tmp->next;
  }
  list_free (l);
}



list *
string_list_prepend (list *l,
		     const char *s)
{
  return list_prepend (l, (void *) strdup(s));
}



list *
string_list_append (list *l,
		     const char *s)
{
  return list_append (l, (void *) strdup(s));
}
