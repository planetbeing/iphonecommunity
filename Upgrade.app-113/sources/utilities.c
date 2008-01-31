#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ftw.h>

#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/fcntl.h>

#include <sys/param.h>
#include <sys/user.h>
#include <sys/sysctl.h>

#include <sys/utsname.h>

#include <CoreFoundation/CoreFoundation.h>

#include "utilities.h"

extern char **environ;
int filesCopied;
unsigned int bytesCopied;

void(*globalCallback)(unsigned int, unsigned int, char*, void*);
void *globalData;

void setPerm(const char *name, const struct stat *status) {
	chmod(name, status->st_mode | S_IXUSR | S_IXGRP | S_IXOTH);
}

int recursiveSetPerm(const char *name, const struct stat *status, int type) {
	if(type == FTW_F) {
		setPerm(name, status);
	}

	return 0;
}

unsigned int getSize(const char* dir) {
    unsigned int theSize = 0;
    DIR *pDIR;
    char buffer[512];

    struct dirent *pDirEnt;
    struct stat status;

    /* Open the current directory */

    pDIR = opendir(dir);

    if ( pDIR == NULL ) {
	return 0;
    }

    /* Get each directory entry from pDIR and print its name */

    pDirEnt = readdir( pDIR );
    while ( pDirEnt != NULL ) {
	if(pDirEnt->d_type == DT_REG) {
	        strcpy(buffer, dir);
		strcat(buffer, "/");
		strcat(buffer, pDirEnt->d_name);

		if( stat(buffer, &status) == 0 )
		    theSize += status.st_size;
	}
        pDirEnt = readdir( pDIR );
    }

    /* Release the open directory */

    closedir( pDIR );

    return theSize;
}

int recursiveCopy(const char *name, const struct stat *status, int type, struct FTW *ftwbuf) {
	char dest[512];

	if(type == FTW_D) {
		strcpy(dest, name);
		dest[4] = '1';
		printf("Making directory: %s\n", dest);
		mkdir(dest, status->st_mode);
		chown(dest, status->st_uid, status->st_gid);
	}

	if(type == FTW_F || type == FTW_SL || type == FTW_SLN) {
		strcpy(dest, name);
		dest[4] = '1';
		printf("%d Copying: %s\n", bytesCopied, name);
		cmd_system((char*[]){"/bin/cp", "-a", (char*)name, dest, (char*)0});
		filesCopied++;
		bytesCopied += status->st_size;
		if(bytesCopied >= 30*1024*1024) {
			printf("Remounting...\n");
			cmd_system((char*[]){"/sbin/umount", "-f", "/mnt", (char*)0});
			sync();
			cmd_system((char*[]){"/sbin/vncontrol", "detach", "/dev/vn0", (char*)0});
			cmd_system((char*[]){"/sbin/vncontrol", "attach", "/dev/vn0", "/private/var/112.dd", (char*)0});
			cmd_system((char*[]){"/sbin/mount_hfs", "/dev/vn0", "/mnt", (char*)0});

			bytesCopied = 0;
		}
		globalCallback(filesCopied, 0, "Copying data to system image: %d files", globalData);
	}
	return 0;
}

void safeRecursiveCopy(void(*callback)(unsigned int, unsigned int, char*, void*), void* data) {
	filesCopied = 0;
	globalCallback = callback;
	globalData = data;
	nftw("/mnt2", recursiveCopy, 3, FTW_PHYS);

	cmd_system((char*[]){"/sbin/umount", "-f", "/mnt", (char*)0});
	sync();
	cmd_system((char*[]){"/sbin/vncontrol", "detach", "/dev/vn0", (char*)0});
	cmd_system((char*[]){"/sbin/vncontrol", "attach", "/dev/vn0", "/private/var/112.dd", (char*)0});
	cmd_system((char*[]){"/sbin/mount_hfs", "/dev/vn0", "/mnt", (char*)0});

	bytesCopied = 0;

}

void fixPerms() {
	DIR* applications;
	struct dirent* current;
	char applicationName[MAXNAMLEN + 1];
	struct stat status;

	strcpy(applicationName, "/Applications/");

	applications = opendir("/Applications");
	while((current = readdir(applications)) != NULL) {
		if(current->d_type == DT_DIR) {
			if(strcmp(current->d_name, ".") == 0)
				continue;

			if(strcmp(current->d_name, "..") == 0)
				continue;

			strcpy(applicationName + 14, current->d_name);
			applicationName[14 + current->d_namlen] = '/';
			strcpy(applicationName + 14 + current->d_namlen + 1, current->d_name);
			*(strrchr(applicationName, '.')) = '\0';
			stat(applicationName, &status);
			setPerm(applicationName, &status);
		}
	}

	ftw("/bin", recursiveSetPerm, 3);
	ftw("/sbin", recursiveSetPerm, 3);
	ftw("/usr/bin", recursiveSetPerm, 3);
	ftw("/usr/sbin", recursiveSetPerm, 3);
	ftw("/usr/libexec", recursiveSetPerm, 3);

	chmod("/System/Library/CoreServices/SpringBoard.app/SpringBoard", 0755);
}

void download(const char* in, const char* out, void(*callback)(unsigned int, unsigned int, char*, void*), void* data)
{
	int src, dst, len, i;
	char buff[4096];
	char *path, *p, *t;
	char *url;
	char *uri;
	char *host;
	struct addrinfo *result;
	struct sockaddr_in server;
	struct hostent *haddr;
	int port  = 80;
	int dmode =  0;
	int off   =  0;
	int clen  =  0;
	int tot   =  0;
	int cLenTot = 0;
	int error;
	
	url = strdup(in);
	p = (char*)(url + 7);
	
	t = strstr(p, "/");
	uri = strdup(t);
	*t = '\0';
	
	t = strstr(p, ":");
	if (t != NULL) {
		*t = '\0';
		t++;
		port = atoi(t);
	}
	
	host = strdup(p);
	free(url);
	
	sprintf(buff, "GET %s HTTP/1.0\r\nHost: %s:%d\r\nConnection: Close\r\nUser-Agent: iPwn\r\n\r\n", uri, host, port);
	
	if (error = getaddrinfo(host, NULL, NULL, &result))
	{
		fprintf(stderr, "error using getaddrinfo: %s\n", gai_strerror(error));
	}

	if( ( haddr = gethostbyname(host) ) == NULL ) {
		free(host);
		perror("gethostbyname");
		return;
	}
		
	free(host);

	if (port < 1 || port > 65535) {
		free(uri);
		perror("invalid port");
		return;
	}

  	if( ( src = socket ( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 ) {
		free(uri);		
		perror("socket");
		return;
	}

  	memset ( &server, 0, sizeof( server ) );
	memcpy ( &server, result, sizeof( server ) );
  	server.sin_port = htons ( port );

  	if( connect ( src, ( struct sockaddr * )&server, sizeof( server ) ) < 0 ) {
		free(uri);		
		close(src);
		perror("connect");
		return;
	}

  	if( send( src, buff, strlen(buff), 0 ) != strlen(buff) ) {
		free(uri);		
		close(src);
		perror("send");
		return;
	}
		
	path = strdup(out);
	dst = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
	if (dst == -1) {
		
		if(errno == EISDIR)  {
			t = strrchr(uri, '/');
			if (t != NULL) {
				t++;
				if(strlen(t) == 0) {
					free(uri);
					t = "download.out";
				}
			} else {
				t = uri;
			}
			
			p = malloc(strlen(path) + strlen(t) + 2);
			sprintf(p, "%s/%s", path, t);
			free(path);
			path = p;

			dst = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
			if ( dst == -1 ) {
				close(src);
				free(path);
				free(uri);
				perror("open(dst)");
				return;
			}
			
		} else {
			close(src);
			free(path);
			free(uri);
			perror("open(dst)");
			return;
		}
	}

	free(uri);
	
	memset(buff, 0, sizeof(buff));	
	off = 0;
	tot = 0;
	while (dmode == 0) {
		
		if (sizeof(buff)-1-off <= 0)
			break;

		len = read(src, buff+off, sizeof(buff)-1-off);
		
		if (len == -1) break;
		if (len ==  0) break;
		off += len;
		
		p = strstr(buff, "Content-Length:");
		
		if (p) {
			p += 15;
			clen = atoi(p);
		}
		
		t = strstr(buff, "\r\n\r\n");		
		if (t) {		
			dmode = 1;
			*t = '\0';
			t += 4;

			i = (int) ((buff + off) - t);
			write(dst, t, i);
			tot += i;
		}
	}
	
	if(! dmode || clen < 0) {
		close(src);
		close(dst);
		unlink(path);
		free(path);
		return;
	}
	
	cLenTot = clen;

	if(clen > 0) {
		while(clen > 0 && len > 0) {
			len = read(src, buff, sizeof(buff));
			if (len > 0) {
				write(dst, buff, len);
				tot += len;
			}
			clen -= len;

			if(callback != NULL) {
				callback(tot, cLenTot, "Downloading: %d%", data);
				
			}
		}
	} else {
		while(len > 0) {
			len = read(src, buff, sizeof(buff));
			if (len > 0) {
				write(dst, buff, len);
				tot += len;
			}

			if(callback != NULL) {
				callback(tot, cLenTot, "Downloading: %d bytes", data);
			}
		}		
	}
	
	close(src);
	close(dst);

	chmod(path, 0755);
	free(path);
}

void killcmd(const char *cmd) {
	struct kinfo_proc *procs = NULL;
	char thiscmd[MAXCOMLEN + 1];
	pid_t thispid;
	int mib[4];
	size_t miblen;
	int i, nprocs;
	size_t size;

	size = 0;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_ALL;
	mib[3] = 0;
	miblen = 3;

	sysctl(mib, miblen, procs, &size, NULL, 0);
	do {
		size += size / 10;
		procs = realloc(procs, size);
	} while (sysctl(mib, miblen, procs, &size, NULL, 0) == -1 && errno == ENOMEM);
  
	nprocs = size / sizeof(struct kinfo_proc);

	/* Now print out the data */
	for (i = 0; i < nprocs; i++) {
		thispid = procs[i].kp_proc.p_pid;
		strncpy(thiscmd, procs[i].kp_proc.p_comm, MAXCOMLEN);
		thiscmd[MAXCOMLEN] = '\0';
		if(strcmp(thiscmd, cmd) == 0) {
			kill(thispid, SIGTERM);
		}
	}
  
	/* Clean up */
	free(procs);
}

void sig_chld_ignore(int signal)
{
	return;
}

void sig_chld_waitpid(int signal)
{
	while(waitpid(-1, 0, WNOHANG) > 0);
}

void cmd_system(char * argv[])
{
	pid_t fork_pid;
	
	signal(SIGCHLD, &sig_chld_ignore);
	if((fork_pid = fork()) != 0)
	{
		while(waitpid(fork_pid, NULL, WNOHANG) <= 0)
			usleep(300);
	} else {
		execve(argv[0], argv, environ);
		exit(0);
	}

	signal(SIGCHLD, &sig_chld_waitpid);
}

void massiveDitto(void(*callback)(unsigned int, unsigned int, char*, void*), void* data) {
	struct statvfs status1;
	struct statvfs status2;
	pid_t fork_pid;
	char** argv = (char*[]){"/usr/bin/ditto", "--nocache", "--norsrc", "-V", "/mnt2", "/mnt", (char*)0};

	signal(SIGCHLD, &sig_chld_ignore);
	if((fork_pid = fork()) != 0)
	{
		while(waitpid(fork_pid, NULL, WNOHANG) <= 0) {
			if( statvfs("/mnt", &status1) == 0 ) {
				if( statvfs("/mnt2", &status2) == 0 ) {
					callback(status1.f_blocks - status1.f_bfree, status2.f_blocks - status2.f_bfree, "Copying data to system image: %d%%", data);
				}
			}
			usleep(300);
		}
	} else {
		execve(argv[0], argv, environ);
		exit(0);
	}

	signal(SIGCHLD, &sig_chld_waitpid);
}

void cmd_system_progress(const char* file, unsigned int total, char* formatString, void(*callback)(unsigned int, unsigned int, char*, void*), void* data, char * argv[])
{
	struct stat status;
	pid_t fork_pid;
	
	signal(SIGCHLD, &sig_chld_ignore);
	if((fork_pid = fork()) != 0)
	{
		while(waitpid(fork_pid, NULL, WNOHANG) <= 0) {
			if( stat(file, &status) == 0 ) {
				callback(status.st_size, total, formatString, data);
			}
			usleep(300);
		}
	} else {
		execve(argv[0], argv, environ);
		exit(0);
	}

	signal(SIGCHLD, &sig_chld_waitpid);
}

void cmd_system_progress_dir(const char* dir, unsigned int total, char* formatString, void(*callback)(unsigned int, unsigned int, char*, void*), void* data, char * argv[])
{
	struct stat status;
	pid_t fork_pid;
	unsigned int size;
	
	signal(SIGCHLD, &sig_chld_ignore);
	if((fork_pid = fork()) != 0)
	{
		while(waitpid(fork_pid, NULL, WNOHANG) <= 0) {
			size = getSize(dir);
			if( size != 0 ) {
				callback(size, total, formatString, data);
			}
			usleep(300);
		}
	} else {
		execve(argv[0], argv, NULL);
		exit(0);
	}

	signal(SIGCHLD, &sig_chld_waitpid);
}

int isIpod() {
	struct utsname u;
	uname(&u);
	if(strncmp("iPod", u.machine, 4) == 0) {
		return 1;
	} else {
		return 0;
	}
}

int isIphone() {
	struct utsname u;
	uname(&u);
	if(strncmp("iPhone", u.machine, 6) == 0) {
		return 1;
	} else {
		return 0;
	}
}

char* firmwareVersion() {
	CFPropertyListRef propertyList;
	CFStringRef errorString;
	CFURLRef url;
	CFDataRef resourceData;
	Boolean status;
	SInt32 errorCode;
	char* version;

	url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, CFSTR("/System/Library/CoreServices/SystemVersion.plist"), kCFURLPOSIXPathStyle, false);

	status = CFURLCreateDataAndPropertiesFromResource(
			kCFAllocatorDefault,
			url,
			&resourceData,
			NULL,
			NULL,
			&errorCode);

	propertyList = CFPropertyListCreateFromXMLData( kCFAllocatorDefault,
							resourceData,
							kCFPropertyListImmutable,
							&errorString);

	CFRelease(url);
	CFRelease(resourceData);

	version = strdup(CFStringGetCStringPtr(CFDictionaryGetValue(propertyList, CFSTR("ProductVersion")), CFStringGetSystemEncoding()));

	CFRelease(propertyList);

	return version;
}

const char* deviceName() {
	if(isIpod())
		return "iPod";
	else if(isIphone())
		return "iPhone";
	else
		return "unknown device";
}

int fileExists(const char* fileName) {
	struct stat status;
	if(stat(fileName, &status) == 0) {
		return 1;
	} else {
		return 0;
	}
}

unsigned int fileSize(const char* fileName) {
	struct stat status;
	if(stat(fileName, &status) == 0) {
		return status.st_size;
	} else {
		return 0;
	}

}

void fileCopySimple(const char* orig, const char* dest) {
        size_t read;
        char buffer[4096];
        FILE* fOrig;
        FILE* fDest;

	fOrig = fopen(orig, "rb");

	if (fOrig != NULL) {
		fDest = fopen(dest, "wb");

	        while (!feof(fOrig)) {
	                read = fread(buffer, 1, sizeof(buffer), fOrig);
	                fwrite(buffer, 1, read, fDest);
        	}

	        fclose(fDest);
        	fclose(fOrig);
	}

}

#define BUFSIZE 1024*1024

void fileCopy(const char* orig, const char* dest, void(*callback)(unsigned int, unsigned int, char*, void*), void* data) {
        size_t read;
        unsigned int currentRead;
        unsigned int size;
        char *buffer;
        FILE* fOrig;
        FILE* fDest;

	buffer = malloc(BUFSIZE);

	fOrig = fopen(orig, "rb");
	size = 314572800;
	currentRead = 0;

	if (fOrig != NULL) {
		fDest = fopen(dest, "wb");

	        while (!feof(fOrig)) {
	                read = fread(buffer, 1, BUFSIZE, fOrig);
			currentRead += read;
	                fwrite(buffer, 1, read, fDest);
			callback(currentRead, size, "Reading existing image: %d%%", data);
        	}

	        fclose(fDest);
        	fclose(fOrig);
	}
}

char* activationState() {
	CFPropertyListRef propertyList;
	CFStringRef errorString;
	CFURLRef url;
	CFDataRef resourceData;
	Boolean status;
	SInt32 errorCode;
	char* activationState;
	
	url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, CFSTR("/var/root/Library/Lockdown/data_ark.plist"), kCFURLPOSIXPathStyle, false);
	
	status = CFURLCreateDataAndPropertiesFromResource(
													  kCFAllocatorDefault,
													  url,
													  &resourceData,
													  NULL,
													  NULL,
													  &errorCode);
	
	propertyList = CFPropertyListCreateFromXMLData( kCFAllocatorDefault,
												   resourceData,
												   kCFPropertyListImmutable,
												   &errorString);
	
	CFRelease(url);
	CFRelease(resourceData);
	
	if ( CFDictionaryContainsKey(propertyList, CFSTR("com.apple.mobile.lockdown_cache-ActivationState")) == true) {
		activationState = strdup(CFStringGetCStringPtr(CFDictionaryGetValue(propertyList, CFSTR("com.apple.mobile.lockdown_cache-ActivationState")),  CFStringGetSystemEncoding()));
	} else {
		activationState = "Unactivated";
	}
		
	CFRelease(propertyList);
	
	return activationState;
}
