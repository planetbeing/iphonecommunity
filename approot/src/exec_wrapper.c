#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

int main(int argc, char *argv[])
{
	char *real_path;
	unsigned long pathlen;
	char **new_argv;
	int arg_len;
	int cnt;

        if(argc < 1 )
		return(0);
	/* Called path + ".real" + NULL */
	pathlen = strlen(argv[0]) + 5 + 1;
	real_path = (char *)malloc(pathlen);
	snprintf(real_path, pathlen, "%s.real",argv[0]);
	new_argv = (char **)malloc(argc * sizeof(char **)+2);
	new_argv[0] = (char *)malloc(17);
	strcpy(new_argv[0], "/usr/libexec/vsu");
	new_argv[1] = real_path;
	for(cnt=1;cnt<argc;cnt++)
	{
		arg_len = strlen(argv[cnt]+1);
		new_argv[cnt+1] = (char *)malloc(arg_len);
		strcpy(new_argv[cnt+1], argv[cnt]);
	}
	new_argv[cnt+1] = (char *)NULL;
	execv("/usr/libexec/vsu",new_argv);
	return(0);
}
