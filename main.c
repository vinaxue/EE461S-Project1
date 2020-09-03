#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

char **parseString(char *str);

int main()
{
	int cpid;
	char *inString;
	char **parsedcmd;

	while (inString = readline("# "))
	{
		parsedcmd = parseString(inString);
		cpid = fork();
		if (cpid == 0)
		{
			execvp(parsedcmd[0], parsedcmd);
		}
		else
		{
			wait((int *)NULL);
		}
	}
}

char **parseString(char *str)
{
	char *tokens[64] = {NULL};
	char *cl_copy, *to_free, *token, *save_ptr;
	int i;

	cl_copy = to_free = strdup(str);

	i = 0;
	while ((token = strtok_r(cl_copy, " ", &save_ptr)))
	{
		tokens[i] = token;
		i++;
		cl_copy = NULL;
	}

	i = 0;
	while(tokens[i]){
		printf("token[%d] is %s\n", i, tokens[i]);
		i++;
	}
	// free(to_free);
	return tokens;
}