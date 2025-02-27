#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>

struct Process
{
	int pid;
	int pgid;
	char **argv;
	//2, stopped, 1 = running, 0 = done, -1 = remove
	int state;
} typedef process;

struct Job
{
	int jobid;

	//2, stopped, 1 = running, 0 = done, -1 = remove
	int state;
	char *cmd;
	process *plist[2];
	int nproc;
	bool fg;
} typedef job;

job **jobs;
int jobNo;
int jobsLength;
int currentFgJob = 0;
extern int errno;

void parseString(char *str, char **tokens, char **to_free);
bool fileRedirection(char **parsedcmd, bool *invalidFile);
void processCommand(char **parsedcmd);
void cleanUpJobs();
void printJobs();
void printBgJobs();
void sigHandler(int signo);

int main()
{
	int cpid;
	int pipefd[2];
	int status;
	char *inString;
	char *parsedcmd[2001] = {NULL};
	char *to_free;

	jobNo = 1;
	jobsLength = 0;
	jobs = (job **)malloc(20 * sizeof(job *));

	for (int i = 0; i < 20; i++)
	{
		jobs[i] = (job *)malloc(sizeof(job));
		jobs[i]->nproc = 0;
	}

	signal(SIGINT, sigHandler);
	signal(SIGTSTP, sigHandler);
	signal(SIGCHLD, sigHandler);

	// signal(SIGTTOU, SIG_IGN);
	// signal(SIGTTIN, SIG_IGN);

	while (inString = readline("# "))
	{
		bool piping = false;
		bool fg = true;
		bool jobCmd = false;
		bool fgCmd = false;
		bool bgCmd = false;

		parseString(inString, parsedcmd, &to_free);

		int counter = 0;
		while (parsedcmd[counter] != (char *)NULL)
		{
			counter++;
		}

		if (counter == 0)
			continue;

		int i = 0;
		char *cmdL[2001] = {NULL};
		char *cmdR[2001] = {NULL};
		int l = 0;

		while (parsedcmd[l] != (char *)NULL)
			l++;

		if (strcmp(parsedcmd[l - 1], "&") == 0)
		{
			fg = false;
			parsedcmd[l - 1] = (char *)NULL;
			l--;
		}

		while (parsedcmd[i] != (char *)NULL)
		{
			if (strcmp(parsedcmd[i], "jobs") == 0)
			{
				jobCmd = true;
				break;
			}
			if (strcmp(parsedcmd[i], "|") == 0)
			{
				int j;
				for (j = 0; j < i; j++)
				{
					cmdL[j] = parsedcmd[j];
				}
				cmdL[j] = (char *)NULL;

				for (j = i + 1; j < l; j++)
				{
					cmdR[j - (i + 1)] = parsedcmd[j];
				}
				cmdR[j - (i + 1)] = (char *)NULL;

				piping = true;
			}
			else if (strcmp(parsedcmd[i], "fg") == 0)
			{
				fgCmd = true;
			}
			else if (strcmp(parsedcmd[i], "bg") == 0)
			{
				bgCmd = true;
			}

			i++;
		}

		if (!jobCmd && !bgCmd && !fgCmd)
		{
			// printf("job length: %d", jobsLength);
			jobs[jobsLength]->jobid = jobNo;
			jobs[jobsLength]->cmd = inString;
			jobs[jobsLength]->state = 1;
			jobs[jobsLength]->fg = fg;

			if (piping)
				jobs[jobsLength]->nproc = 2;
			else
				jobs[jobsLength]->nproc = 1;

			for (int i = 0; i < jobs[jobsLength]->nproc; i++)
			{
				jobs[jobsLength]->plist[i] = (process *)malloc(sizeof(process));
				jobs[jobsLength]->plist[i]->state = 1;
			}

			jobsLength++;
			jobNo++;
		}

		if (jobCmd)
		{
			printJobs();
		}
		else if (fgCmd)
		{
			for (int i = jobsLength - 1; i >= 0; i--)
			{
				if (!jobs[i]->fg || jobs[i]->state == 2)
				{
					char *recent = i == jobsLength - 1 ? "+" : "-";
					char *s = "Running";
					kill(jobs[i]->plist[0]->pgid, SIGCONT);

					jobs[i]->fg = true;
					printf("[%d]%s  %s        %s \n", jobs[i]->jobid, recent, s, jobs[i]->cmd);
					jobs[i]->state = 1;
					for (int j = 0; j < jobs[i]->nproc; j++)
					{
						jobs[i]->plist[j]->state = 1;
					}

					int status;

					currentFgJob = jobs[i]->plist[0]->pgid;
					pid_t result = waitpid(-1 * jobs[i]->plist[0]->pgid, &status, WUNTRACED);
					currentFgJob = 0;

					signal(SIGINT, sigHandler);
					signal(SIGTSTP, sigHandler);
					signal(SIGCHLD, sigHandler);

					break;
				}
			}
		}
		else if (bgCmd)
		{
			for (int i = jobsLength - 1; i >= 0; i--)
			{
				if (jobs[i]->state == 2)
				{
					char *recent = i == jobsLength - 1 ? "+" : "-";
					char *s = "Running";

					// signal(SIGINT, sigHandler);
					// signal(SIGTSTP, sigHandler);
					// signal(SIGCHLD, sigHandler);
					kill(-1 * jobs[i]->plist[0]->pgid, SIGCONT);

					if (strcmp(jobs[i]->cmd + strlen(jobs[i]->cmd) - 1, " ") != 0)
						strncat(jobs[i]->cmd, " ", 1);
					strncat(jobs[i]->cmd, "&", 1);
					printf("[%d]%s  %s        %s \n", jobs[i]->jobid, recent, s, jobs[i]->cmd);

					jobs[i]->state = 1;
					jobs[i]->fg = false;
					for (int j = 0; j < jobs[i]->nproc; j++)
					{
						jobs[i]->plist[j]->state = 1;
					}
					break;
				}
			}
		}
		else if (piping)
		{
			pipe(pipefd);
			pid_t cpidL = fork();
			// printf("cpidL %d \n", cpidL);
			if (cpidL == 0)
			{

				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				signal(SIGCHLD, SIG_DFL);
				setpgid(0, 0);
				// printf("left child pid: %d \n", getpid());
				// printf("left child pgid: %d \n", getpgid(getpid()));
				close(pipefd[0]);
				dup2(pipefd[1], STDOUT_FILENO);
				processCommand(cmdL);
			}
			else
			{
				jobs[jobsLength - 1]->plist[0]->pid = cpidL;
				jobs[jobsLength - 1]->plist[0]->pgid = cpidL;
				///////////////////////////////////////////////////////////////////////
				int status;
				// printf("parent pid: %d \n", getpid());
				currentFgJob = cpidL;
				cpid = fork();
				if (cpid == 0)
				{
					// int errnum;
					signal(SIGINT, SIG_DFL);
					signal(SIGTSTP, SIG_DFL);
					signal(SIGCHLD, SIG_DFL);
					int n = setpgid(0, cpidL);
					// if (n == -1)
					// {
					// 	errnum = errno;
					// 	fprintf(stderr, "Error waitpid: %s\n", strerror(errnum));
					// }
					// printf("cpidL %d \n", cpidL);
					// printf("right child pid: %d \n", getpid());
					// printf("right child pgid: %d \n", getpgid(getpid()));
					close(pipefd[1]);
					dup2(pipefd[0], STDIN_FILENO);
					processCommand(cmdR);
				}
				else
				{
					jobs[jobsLength - 1]->plist[1]->pid = cpid;
					jobs[jobsLength - 1]->plist[0]->pgid = cpidL;
					close(pipefd[0]);
					close(pipefd[1]);
					if (fg)
					{

						int status;

						signal(SIGINT, sigHandler);
						signal(SIGTSTP, sigHandler);
						signal(SIGCHLD, sigHandler);

						for (int i = 0; i < jobsLength; i++)
						{
							if (jobs[i]->fg)
							{
								waitpid(cpid, &status, WNOHANG);

								if (jobs[i]->state == 0 || WIFSIGNALED(status))
								{
									jobs[i]->state = -1;
								}
								else if (WIFSTOPPED(status))
								{
									//stopped
									kill(-1 * jobs[i]->plist[0]->pid, SIGTSTP);
									jobs[i]->state = 2;
									jobs[i]->fg = false;
								}
								else if (WIFEXITED(status))
								{
									jobs[i]->state = 0;
								}
								else
								{
									jobs[i]->state = 1;
								}
							}
						}
					}
				}

				if (fg)
				{
					waitpid(cpidL, &status, WUNTRACED);
					waitpid(cpid, &status, WUNTRACED);
					currentFgJob = 0;

					signal(SIGINT, sigHandler);
					signal(SIGTSTP, sigHandler);
					signal(SIGCHLD, sigHandler);

					for (int i = 0; i < jobsLength; i++)
					{
						if (jobs[i]->fg)
						{
							waitpid(cpid, &status, WNOHANG);

							if (jobs[i]->state == 0 || WIFSIGNALED(status))
							{
								jobs[i]->state = -1;
							}
							else if (WIFSTOPPED(status))
							{
								//stopped
								kill(-1 * jobs[i]->plist[0]->pid, SIGTSTP);
								jobs[i]->state = 2;
								jobs[i]->fg = false;
							}
							else if (WIFEXITED(status))
							{
								jobs[i]->state = 0;
							}
							else
							{
								jobs[i]->state = 1;
							}
						}
					}
				}
			}
		}
		else
		{
			cpid = fork();
			if (cpid == 0)
			{
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				signal(SIGCHLD, SIG_DFL);
				setpgid(0, 0);
				processCommand(parsedcmd);
			}
			else
			{
				jobs[jobsLength - 1]->plist[0]->pid = cpid;
				jobs[jobsLength - 1]->plist[0]->pgid = cpid;
				if (fg)
				{
					int status;

					currentFgJob = cpid;
					pid_t result = waitpid(cpid, &status, WUNTRACED);
					currentFgJob = 0;

					signal(SIGINT, sigHandler);
					signal(SIGTSTP, sigHandler);
					signal(SIGCHLD, sigHandler);

					for (int i = 0; i < jobsLength; i++)
					{
						if (jobs[i]->fg)
						{
							waitpid(cpid, &status, WNOHANG);

							if (jobs[i]->state == 0 || WIFSIGNALED(status))
							{
								jobs[i]->state = -1;
							}
							else if (WIFSTOPPED(status))
							{
								//stopped
								kill(-1 * jobs[i]->plist[0]->pid, SIGTSTP);
								jobs[i]->state = 2;
								jobs[i]->fg = false;
							}
							else if (WIFEXITED(status))
							{
								jobs[i]->state = 0;
							}
							else
							{
								jobs[i]->state = 1;
							}
						}
					}
				}
			}
		}

		printBgJobs();

		cleanUpJobs();
	}
	free(to_free);
	// printf("to free in main: %d \n", **to_free);

	for (int i = 0; i < 20; i++)
	{
		if (jobs[i]->nproc > 0)
		{
			for (int i = 0; i < jobs[i]->nproc; i++)
			{
				free(jobs[i]->plist[i]);
			}
		}
		free(jobs[i]);
	}

	free(jobs);
}

void parseString(char *str, char **tokens, char **to_free)
{
	char *cl_copy, *token, *save_ptr;
	int i;

	cl_copy = *to_free = strdup(str);
	// printf("to free in parse: %d \n", **to_free);

	i = 0;
	while ((token = strtok_r(cl_copy, " ", &save_ptr)))
	{
		tokens[i] = token;
		i++;
		cl_copy = NULL;
	}

	tokens[i] = (char *)NULL;

	i = 0;
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
				fprintf(stderr, "bash: %s: No such file or directory\n", parsedcmd[i + 1]);
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
	int n = 0;
	char *fullCmd = "";

	if (fileRedirect)
	{
		if (!invalidFile)
		{
			char *cmd[2001] = {NULL};
			int i = 0;
			while (parsedcmd[i] != (char *)NULL)
			{
				if (strcmp(parsedcmd[i], ">") != 0 && strcmp(parsedcmd[i], "<") != 0 && strcmp(parsedcmd[i], "2>") != 0)
				{
					cmd[i] = parsedcmd[i];
					strcat(fullCmd, parsedcmd[i]);
					strcat(fullCmd, " ");
				}
				else
				{
					break;
				}
				i++;
			}

			n = execvp(cmd[0], cmd);
		}
	}
	else
	{
		int i = 0;
		while (parsedcmd[i] != (char *)NULL)
		{
			strcat(fullCmd, parsedcmd[i]);
			strcat(fullCmd, " ");
			i++;
		}
		n = execvp(parsedcmd[0], parsedcmd);
	}

	if (n == -1)
	{
		for (int i = 0; i < 20; i++)
		{
			if (strcmp(jobs[i]->cmd, fullCmd) == 0)
			{
				jobs[i]->state = -1;
			}
		}
	}
}

void cleanUpJobs()
{
	int removeIndex[jobsLength];

	int counter = 0;
	for (int i = 0; i < jobsLength; i++)
	{
		if (jobs[i]->state == -1)
		{
			removeIndex[counter] = i;
			counter++;
		}
	}

	for (int i = 0; i < counter; i++)
	{
		for (int j = removeIndex[i]; j < jobsLength - 1; j++)
		{
			jobs[j] = jobs[j + 1];
		}

		for (int j = i + 1; j < counter; j++)
		{
			removeIndex[j] = removeIndex[j] - 1;
		}
	}

	jobsLength -= counter;

	jobNo = 1;
	if (jobsLength > 0)
	{
		if (jobs[jobsLength - 1]->jobid >= jobNo)
		{
			jobNo = jobs[jobsLength - 1]->jobid + 1;
		}
	}

	job **tmp = (job **)realloc(jobs, sizeof(job *) * 20);
	if (tmp)
		jobs = tmp;
}

void printJobs()
{
	for (int i = 0; i < jobsLength; i++)
	{
		char *recent = i == jobsLength - 1 ? "+" : "-";
		char *s = "";

		if (jobs[i]->state == 0)
		{
			s = "Done";
		}
		else if (jobs[i]->state == 1)
		{
			s = "Running";
		}
		else if (jobs[i]->state == 2)
		{
			s = "Stopped";
		}
		else
		{
			jobs[i]->state = -1;
			s = "";
		}

		if (strcmp(s, "") != 0)
			printf("[%d]%s  %s        %s \n", jobs[i]->jobid, recent, s, jobs[i]->cmd);

		if (jobs[i]->state == 0)
		{
			jobs[i]->state = -1;
		}
	}
}

void printBgJobs()
{
	for (int i = 0; i < jobsLength; i++)
	{
		// printf("%s %d\n\n", jobs[i]->cmd, jobs[i]->state);
		if (!jobs[i]->fg)
		{
			char *recent = i == jobsLength - 1 ? "+" : "-";
			char *s = "";

			if (jobs[i]->state == 0)
			{
				s = "Done";
			}

			if (strcmp(s, "Done") == 0)
				printf("[%d]%s  %s        %s \n", jobs[i]->jobid, recent, s, jobs[i]->cmd);
		}
		if (jobs[i]->state == 0)
		{
			jobs[i]->state = -1;
		}
	}
}

void sigHandler(int signo)
{
	int status;
	int pid;
	int errnum;

	if (currentFgJob > 0)
	{
		if (signo == SIGTSTP)
		{
			for (int i = 0; i < jobsLength; i++)
			{
				if (currentFgJob == jobs[i]->plist[0]->pgid)
				{
					jobs[i]->fg = false;
					jobs[i]->state = 2;
					for (int j = 0; j < jobs[i]->nproc; j++)
					{
						jobs[i]->plist[j]->state = 2;
					}
				}
			}
		}
		kill(-1 * currentFgJob, signo);
	}

	switch (signo)
	{
	case SIGINT:
		//ctrl-c
		//quit current foreground process only
		printf("\n");
		break;
	case SIGTSTP:
		//ctrl-z
		//send sigtstp to current foreground process
		printf("\n");
		break;
	case SIGCHLD:
		pid = waitpid(-1, NULL, WNOHANG);
		// if (pid == -1)
		// {
		// 	errnum = errno;
		// 	fprintf(stderr, "Error waitpid: %s\n", strerror(errnum));
		// }

		// printf("caught sigchld %d \n", pid);
		for (int i = 0; i < jobsLength; i++)
		{
			if (jobs[i]->nproc == 1)
			{
				if (pid == jobs[i]->plist[0]->pgid)
				{
					// jobs[i]->state = 0;
					// printf("%s \n", jobs[i]->cmd);
					// }
					if (jobs[i]->state == 0 || WIFSIGNALED(status))
					{
						jobs[i]->state = -1;
					}
					else if (WIFSTOPPED(status))
					{
						//stopped
						kill(-1 * jobs[i]->plist[0]->pgid, SIGTSTP);
						jobs[i]->state = 2;
						jobs[i]->fg = false;
					}
					else if (WIFEXITED(status))
					{
						jobs[i]->state = 0;
					}
					else
					{
						jobs[i]->state = 1;
					}
				}
			}
			else
			{
				if (pid == jobs[i]->plist[0]->pid)
				{
					if (jobs[i]->plist[0]->state == 0 || WIFSIGNALED(status))
					{
						jobs[i]->plist[0]->state = -1;
					}
					else if (WIFSTOPPED(status))
					{
						//stopped
						kill(-1 * jobs[i]->plist[0]->pgid, SIGTSTP);
						jobs[i]->plist[0]->state = 2;
						jobs[i]->fg = false;
					}
					else if (WIFEXITED(status))
					{
						jobs[i]->plist[0]->state = 0;
					}
					else
					{
						jobs[i]->plist[0]->state = 1;
					}
				}
				else if (pid == jobs[i]->plist[1]->pid)
				{
					if (jobs[i]->plist[1]->state == 0 || WIFSIGNALED(status))
					{
						jobs[i]->plist[1]->state = -1;
					}
					else if (WIFSTOPPED(status))
					{
						//stopped
						kill(-1 * jobs[i]->plist[0]->pgid, SIGTSTP);
						jobs[i]->plist[1]->state = 2;
						jobs[i]->fg = false;
					}
					else if (WIFEXITED(status))
					{
						jobs[i]->plist[1]->state = 0;
					}
					else
					{
						jobs[i]->plist[1]->state = 1;
					}
				}

				if (jobs[i]->plist[0]->state == 0 && jobs[i]->plist[1]->state == 0)
				{
					jobs[i]->state = 0;
				}
				else if (jobs[i]->plist[0]->state == 2 && jobs[i]->plist[1]->state == 2)
				{
					jobs[i]->state = 2;
				}
				else if (jobs[i]->plist[0]->state == -1 && jobs[i]->plist[1]->state == -1)
				{
					jobs[i]->state = -1;
				}
				else
				{
					jobs[i]->state = 1;
				}
			}
		}
		break;
	case EOF:
		exit(0);
		break;
	}
	signal(SIGINT, sigHandler);
	signal(SIGTSTP, sigHandler);
	signal(SIGCHLD, sigHandler);
}