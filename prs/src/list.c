/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"



void *
list_get_item (list *l, int item)
{
	while (item-- && l)
		l = l->next;
	if (l)
		return l->data;
	else
		return NULL;
}



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
}



list *
list_append (list *l, void *data)
{
	list *tmp = l; 
	list *new_item = (list *) malloc (sizeof(list));

	assert (new_item != NULL);
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

	assert (new_item != NULL);
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

	assert (l != NULL);
	assert (new_item != NULL);
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

	assert (l != NULL);
	assert (new_item != NULL);
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
	assert (l != NULL);
	assert (item != NULL);
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
	return i;
}



list *
list_copy (list *l)
{
	list *new_list;
	list *tmp;
	list *new_item = NULL, *prev = NULL;
  
	assert (l != NULL);
	assert (new_item != NULL);
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

	while (l && l->next) {
		tmp = l->prev;
		l->prev = l->next;
		l->next = tmp;
		l = l->prev;
	}
	l->next = l->prev;
	l->prev = NULL;
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


char **
string_list_to_array (list *l)
{
	int i;
	char **array;

	assert (l != NULL);
	i = list_length (l);
	array = malloc ((i+1)*sizeof(char *));
	i = 0;
	while (l) {
		array[i] = strdup ((char *) l->data);
		l = l->next;
		i++;
	}
	array[i] = NULL;
	return array;
}
