#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#define BUFSIZE 1024*1024

void fileCopy(const char* orig, const char* dest) {
        size_t read;
        unsigned int currentRead;
        unsigned int size;
        char *buffer;
        FILE* fOrig;
        FILE* fDest;

	int s;
	socklen_t len;
	struct sockaddr_un remote;

	buffer = malloc(BUFSIZE);

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, "/progress.sock");
	len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
	connect(s, (struct sockaddr *)&remote, len);

	fOrig = fopen(orig, "rb");
	fcntl(fileno(fOrig), F_NOCACHE, 1);
	fseek(fOrig, 0, SEEK_END);
	size = ftell(fOrig);
	currentRead = 0;

	fseek(fOrig, 0, SEEK_SET);

	if (fOrig != NULL) {
		fDest = fopen(dest, "wb");
		fcntl(fileno(fDest), F_NOCACHE, 1);

	        while (!feof(fOrig)) {
	                read = fread(buffer, 1, BUFSIZE, fOrig);
			currentRead += read;
	                fwrite(buffer, 1, read, fDest);
			send(s, &currentRead, sizeof(currentRead), 0);
			send(s, &size, sizeof(size), 0);
        	}

	        fclose(fDest);
        	fclose(fOrig);
	}
}

int main(int argc, char** argv) {
	fileCopy("/private/var/disk0s1.dd", "/dev/rdisk0s1");
	sync();
	sync();
	sync();
	reboot(RB_NOSYNC);
}

