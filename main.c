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
void sigHandler(int signo);

struct Job
{
	char *cmd;
	int status;
} typedef job;

int main()
{
	pid_t cpid;
	int pipefd[2];
	char *inString;
	char *parsedcmd[64] = {NULL};
	int status;
	job *foreground = malloc(sizeof(job));
	job *background = malloc(sizeof(job));

	if (signal(SIGINT, sigHandler) == SIG_ERR)
		perror("Couldn't install signal handler");
	// signal(SIGTTOU, SIG_IGN);
	// signal(SIGTTIN, SIG_IGN);

	while (inString = readline("# "))
	{
		bool piping = false;
		bool fg = true;
		bool bg = false;

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
			}
			else if (strcmp(parsedcmd[i], "fg") == 0)
			{
				fg == true;
				bg == false;
			}
			else if (strcmp(parsedcmd[i], "bg") == 0 || strcmp(parsedcmd[i], "&") == 0)
			{
				bg == true;
				fg == false;
			}

			i++;
		}

		if (piping)
		{
			pipe(pipefd);
			pid_t cpidL = fork();
			if (cpidL == 0)
			{
				signal(SIGINT, SIG_DFL);
				// setpgid(0, 0);
				close(pipefd[0]);
				dup2(pipefd[1], STDOUT_FILENO);
				processCommand(cmdL);
			}
			else
			{
				if (bg)
				{
					do
					{
						int w = waitpid(cpid, &status, WNOHANG);
						if (w == -1)
						{
							perror("waitpid");
							exit(EXIT_FAILURE);
						}

						if (WIFEXITED(status))
						{
							printf("exited, status=%d\n", WEXITSTATUS(status));
						}
						else if (WIFSIGNALED(status))
						{
							printf("killed by signal %d\n", WTERMSIG(status));
						}
						else if (WIFSTOPPED(status))
						{
							printf("stopped by signal %d\n", WSTOPSIG(status));
						}
						else if (WIFCONTINUED(status))
						{
							printf("continued\n");
						}
					} while (!WIFEXITED(status) && !WIFSIGNALED(status));
				}
				else
				{
					waitpid(cpidL, NULL, 0);
				}
			}

			cpid = fork();
			if (cpid == 0)
			{
				signal(SIGINT, SIG_DFL);
				// setpgid(0, 0);
				close(pipefd[1]);
				dup2(pipefd[0], STDIN_FILENO);
				processCommand(cmdR);
			}
			else
			{
				close(pipefd[0]);
				close(pipefd[1]);
				if (bg)
				{
					do
					{
						int w = waitpid(cpid, &status, WNOHANG);
						if (w == -1)
						{
							perror("waitpid");
							exit(EXIT_FAILURE);
						}

						if (WIFEXITED(status))
						{
							printf("exited, status=%d\n", WEXITSTATUS(status));
						}
						else if (WIFSIGNALED(status))
						{
							printf("killed by signal %d\n", WTERMSIG(status));
						}
						else if (WIFSTOPPED(status))
						{
							printf("stopped by signal %d\n", WSTOPSIG(status));
						}
						else if (WIFCONTINUED(status))
						{
							printf("continued\n");
						}
					} while (!WIFEXITED(status) && !WIFSIGNALED(status));
				}
				else
				{
					// tcsetpgrp(0, cpidL);
					waitpid(cpid, NULL, 0);
				}
			}
		}
		else
		{
			cpid = fork();
			if (cpid == 0)
			{
				signal(SIGINT, SIG_DFL);
				// printf("%d", setpgid(0, 0));
				processCommand(parsedcmd);
			}
			else
			{
				if (bg)
				{
					do
					{
						int w = waitpid(cpid, &status, WNOHANG);
						if (w == -1)
						{
							perror("waitpid");
							exit(EXIT_FAILURE);
						}

						if (WIFEXITED(status))
						{
							printf("exited, status=%d\n", WEXITSTATUS(status));
						}
						else if (WIFSIGNALED(status))
						{
							printf("killed by signal %d\n", WTERMSIG(status));
						}
						else if (WIFSTOPPED(status))
						{
							printf("stopped by signal %d\n", WSTOPSIG(status));
						}
						else if (WIFCONTINUED(status))
						{
							printf("continued\n");
						}
					} while (!WIFEXITED(status) && !WIFSIGNALED(status));
				}
				else
				{
					waitpid(cpid, NULL, 0);
				}
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

void sigHandler(int signo)
{
	pid_t pid = getpid();

	switch (signo)
	{
	case SIGINT:
		//ctrl-c
		//quit current foreground process only
		printf("\n");
		break;
	case SIGTSTP:
		//ctrl-z
		//send sigtsto to current foreground process
		break;
	case EOF:
		exit(0);
		break;
	}
	signal(SIGINT, sigHandler);
}