#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(int argc, char *argv[])
{
	seteuid(0);
	setuid(0);
	setgid(0);
	//execl("/bin/sh","sh",0);
	execv(argv[1],argv+1);
	return(0);
}
