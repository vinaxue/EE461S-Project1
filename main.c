#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

void parseString(char *str, char **tokens);

int main()
{
	int cpid;
	char *inString;
	char *parsedcmd[64] = {NULL};

	while (inString = readline("# "))
	{
		parseString(inString, parsedcmd);
		cpid = fork();
		if (cpid == 0)
		{
			int i = 0;
			while (parsedcmd[i] != (char *)NULL)
			{
				if (strcmp(parsedcmd[i], ">") == 0)
				{
					fork();
					int ofd = creat(parsedcmd[i + 1], 0644);
					dup2(ofd, 1);
					execlp(parsedcmd[i - 1], parsedcmd[i - 1], (char *)NULL);
				}
				else if (strcmp(parsedcmd[i], "<") == 0)
				{
					fork();
					if (access(parsedcmd[i + 1], F_OK))
					{
						int ofd = open(parsedcmd[i + 1], 0644);
						dup2(ofd,   bgtr5fdxa0);
						execlp(parsedcmd[i - 1], parsedcmd[i - 1], (char *)NULL);
					}
				}
				i++;
			}
			execvp(parsedcmd[0], parsedcmd);
		}
		else
		{
			wait((int *)NULL);
		}
	}
}

void parseString(char *str, char **tokens)
{
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

	tokens[i] = (char *)NULL;

	i = 0;
	// free(to_free);
}