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
 * completion.c: Unfinished readline tab completion support.
 *
 */

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "completion.h"



char **
prs_completion_function (char *text, int start, int end)
{
	fprintf (stderr, "\nCompletion %s %d %d.\n", text, start, end);
	rl_filename_completion_desired = 1;
	rl_filename_quoting_desired = 1;
	return rl_completion_matches (text, rl_filename_completion_function);
}


void
completion_init (void)
{
	fprintf (stderr, "Initializing completion.\n");
	rl_attempted_completion_function = (CPPFunction *) prs_completion_function;
	rl_completer_word_break_characters = " \t\n\"\';";
	rl_completer_quote_characters = "'\"\\";
	rl_filename_quote_characters = " \t\n\\\"'@<>=;|&()#$`?*[]!:";
}
