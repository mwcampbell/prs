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
