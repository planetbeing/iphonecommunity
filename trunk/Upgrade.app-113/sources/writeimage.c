#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#define BUFSIZE 1024*1024

void fileCopy(const char* orig, const char* dest, int s) {
        size_t read;
        unsigned int currentRead;
        unsigned int size;
        char *buffer;
        FILE* fOrig;
        FILE* fDest;

	buffer = malloc(BUFSIZE);

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
	FILE* tmp;
	unsigned int s, s2;
	socklen_t len;
	struct sockaddr_un local;
	struct sockaddr_un remote;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	local.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
	strcpy(local.sun_path, "/private/var/progress.sock");
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
	bind(s, (struct sockaddr *)&local, len);
	listen(s, 1);
	len = sizeof(struct sockaddr_un);

	s2 = accept(s, (struct sockaddr *)&remote, &len);

	tmp = fopen("/private/var/writeStarted", "wb");
	fwrite(&s2, 1, 1, tmp);
	fclose(tmp);

	fileCopy("/private/var/disk0s1.dd", "/dev/rdisk0s1", s2);
	sync();
	sync();
	sync();
	reboot(RB_NOSYNC);
}

