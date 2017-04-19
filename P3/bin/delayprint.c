#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  	sleep(1);
  	printf("print: %d\n", atoi(argv[1]));
}
