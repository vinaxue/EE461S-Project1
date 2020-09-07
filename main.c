#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

void parseString(char *str, char **tokens);
bool fileRedirection(char **parsedcmd, bool *invalidFile);
void processCommand(char **parsedcmd);

int main()
{
	int cpid;
	int pipefd[2];
	char *inString;
	char *parsedcmd[64] = {NULL};

	while (inString = readline("# "))
	{
		bool piping = false;

		parseString(inString, parsedcmd);

		int i = 0;
		char *cmdL[64] = {NULL};
		char *cmdR[64] = {NULL};

		while (parsedcmd[i] != (char *)NULL)
		{
			if (strcmp(parsedcmd[i], "|") == 0)
			{
				int j;
				for (j = 0; j < i; j++)
				{
					cmdL[j] = parsedcmd[j];
				}
				cmdL[j] = (char *)NULL;

				for (j = i + 1; j < sizeof(parsedcmd) / sizeof(char *); j++)
				{
					cmdR[j - (i + 1)] = parsedcmd[j];
				}
				cmdR[j - (i + 1)] = (char *)NULL;

				piping = true;
				break;
			}
			i++;
		}

		// i = 0;
		// while (cmdL[i])
		// {
		// 	printf("cmdL[%d] is %s\n", i, cmdL[i]);
		// 	i++;
		// }

		// i = 0;
		// while (cmdR[i])
		// {
		// 	printf("cmdR[%d] is %s\n", i, cmdR[i]);
		// 	i++;
		// }

		// printf("%d\n", piping);

		if (piping)
		{
			pipe(pipefd);
			cpid = fork();
			if (cpid == 0)
			{
				close(pipefd[0]);
				dup2(pipefd[1], STDOUT_FILENO);
				processCommand(cmdL);
			}
			else
			{
				wait((int *)NULL);
			}

			cpid = fork();
			if (cpid == 0)
			{
				close(pipefd[1]);
				dup2(pipefd[0], STDIN_FILENO);
				processCommand(cmdR);
			}
			else
			{
				close(pipefd[0]);
				close(pipefd[1]);
				wait((int *)NULL);
			}
		}
		else
		{
			cpid = fork();
			if (cpid == 0)
			{
				processCommand(parsedcmd);
			}
			else
			{
				wait((int *)NULL);
			}
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
	// while (tokens[i])
	// {
	// 	printf("token[%d] is %s\n", i, tokens[i]);
	// 	i++;
	// }

	// free(to_free);
}

bool fileRedirection(char **parsedcmd, bool *invalidFile)
{
	bool fileRedirect = false;
	int i = 0;
	while (parsedcmd[i] != (char *)NULL)
	{
		if (strcmp(parsedcmd[i], "<") == 0)
		{
			int ofd = open(parsedcmd[i + 1], 0644);
			if (ofd != -1)
			{
				dup2(ofd, 0);
			}
			else
			{
				*invalidFile = true;
			}
			fileRedirect = true;
		}
		else if (strcmp(parsedcmd[i], ">") == 0)
		{
			int ofd = creat(parsedcmd[i + 1], 0644);
			dup2(ofd, 1);

			fileRedirect = true;
		}
		else if (strcmp(parsedcmd[i], "2>") == 0)
		{

			int ofd = creat(parsedcmd[i + 1], 0644);
			dup2(ofd, 2);

			fileRedirect = true;
		}
		i++;
	}

	return fileRedirect;
}

void processCommand(char **parsedcmd)
{
	bool invalidFile = false;
	bool fileRedirect = fileRedirection(parsedcmd, &invalidFile);

	if (fileRedirect)
	{
		if (!invalidFile)
		{
			char *cmd[64] = {NULL};
			int i = 0;
			while (parsedcmd[i] != (char *)NULL)
			{
				if (strcmp(parsedcmd[i], ">") != 0 && strcmp(parsedcmd[i], "<") != 0 && strcmp(parsedcmd[i], "2>") != 0)
				{
					cmd[i] = parsedcmd[i];
				}
				else
				{
					break;
				}
				i++;
			}

			execvp(cmd[0], cmd);
		}
	}
	else
	{
		execvp(parsedcmd[0], parsedcmd);
	}
}