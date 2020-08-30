#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

char **parseString(char *str);

int main()
{
	int cpid;
	char *inString;
	char **parsedcmd;

	while (inString = readline("cmd:"))
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