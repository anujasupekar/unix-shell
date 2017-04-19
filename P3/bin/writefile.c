#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	sleep(1);
	FILE* fd = fopen(argv[1], "w+");
	fprintf(fd, "file: Hello World\n");
	fclose(fd);
}
