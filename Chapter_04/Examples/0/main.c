#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

int main() 
{
	int c;

	int fd;

	fd=open("myfile.txt",O_RDONLY);

  if (fd<0)
	{
		// perror("fd returned -1 Error:");
		fprintf(stderr,"fd returned %d: errno=%d: %s\n",
		  fd,errno, strerror(errno));
		return 1;
	}

	fprintf(stderr,"MyPID==%d\n",getpid());
	c=getchar();

	return 0;
}
