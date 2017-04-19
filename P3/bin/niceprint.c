#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	pid_t childPID = fork();
	if (childPID == 0) {
  		sleep(1);
  		printf("nice: %d\n", atoi(argv[1]));
	}
}
