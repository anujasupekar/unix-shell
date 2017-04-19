#include <stdio.h>

int main(int argc, char *argv[])
{
  int param;

  if (argc == 2)
  	param = atoi(argv[1]);
  else if (argc == 1)
	param = 1;
  else
	return -1;

  if (param >= 1) {
      fprintf(stdout, "stdout: Hello World\n");
      fflush(stdout);
  }
  if (param >= 2)
      fprintf(stderr, "stderr: Hello World\n");
}
