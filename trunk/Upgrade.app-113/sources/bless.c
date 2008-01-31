#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnioctl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <hfs/hfs_mount.h>

int main(int argc, char* argv[]) {
	struct statfs statfsData;
	char characterDevice[MNAMELEN];
	char systemFolder[MAXPATHLEN];
	char* lastSlash;
	FILE* fDevice;
	int readOnly;
	unsigned int systemFolderId;
	unsigned int osxSystemFolderId;
	struct stat status;
	struct hfs_mount_args hma;

	if(argc != 2) {
		printf("Usage: %s [mount-point]\n", argv[0]);
		return 0;
	}

	if(statfs(argv[1], &statfsData) != 0) {
		perror("statfs");
		return 1;
	}

	strcpy(characterDevice, statfsData.f_mntfromname);
	lastSlash = strrchr(characterDevice, '/');
	*(lastSlash + 1) = 'r';
	strcpy(lastSlash + 2, statfsData.f_mntfromname + (lastSlash - characterDevice) + 1);
	strcpy(systemFolder, statfsData.f_mntonname);
	strcat(systemFolder, "/System/Library/CoreServices");

	readOnly = statfsData.f_flags & MNT_RDONLY;

	if(stat(systemFolder, &status) != 0) {
		perror("stat");
		return 1;
	}

	fDevice = fopen(characterDevice, "r");
	if(fDevice == NULL) {
		perror("fopen");
		return 1;
	}

	if(fseek(fDevice, 0x450, SEEK_CUR) < 0) {
		perror("fseek");
		return 1;
	}

	if(fread(&systemFolderId, 4, 1, fDevice) != 1) {
		perror("fread");
		return 1;
	}

	if(fseek(fDevice, 0x10, SEEK_CUR) < 0) {
		perror("fseek");
		return 1;
	}

	if(fread(&osxSystemFolderId, 4, 1, fDevice) != 1) {
		perror("fread");
		return 1;
	}
	fclose(fDevice);

	printf("%x %x -> ", htonl(systemFolderId), htonl(osxSystemFolderId));
	systemFolderId = htonl(status.st_ino);
	osxSystemFolderId = htonl(status.st_ino);
	printf("%x %x\n", htonl(systemFolderId), htonl(osxSystemFolderId));

	if(unmount(statfsData.f_mntonname, 0) <0 ) {
		perror("unmount");
		return 1;
	}

	fDevice = fopen(characterDevice, "w");
	if(fDevice == NULL) {
		perror("fopen");
		return 1;
	}

	if(fseek(fDevice, 0x450, SEEK_CUR) < 0) {
		perror("fseek");
		return 1;
	}

	if(fwrite(&systemFolderId, 4, 1, fDevice) != 1) {
		perror("fwrite");
		return 1;
	}

	if(fseek(fDevice, 0x10, SEEK_CUR) < 0) {
		perror("fseek");
		return 1;
	}

	if(fwrite(&osxSystemFolderId, 4, 1, fDevice) != 1) {
		perror("fwrite");
		return 1;
	}

	if(fseek(fDevice, statfsData.f_blocks * statfsData.f_bsize - 1024 + 0x50 - ftell(fDevice), SEEK_CUR) < 0) {
		perror("fseek");
		return 1;
	}

	if(fwrite(&systemFolderId, 4, 1, fDevice) != 1) {
		perror("fwrite");
		return 1;
	}

	if(fseek(fDevice, 0x10, SEEK_CUR) < 0) {
		perror("fseek");
		return 1;
	}

	if(fwrite(&osxSystemFolderId, 4, 1, fDevice) != 1) {
		perror("fwrite");
		return 1;
	}

	fclose(fDevice);

	if(readOnly) {
		bzero(&hma, sizeof(hma));
		hma.fspec = statfsData.f_mntfromname;
		if(mount("hfs", statfsData.f_mntonname, MNT_RDONLY, &hma) < 0) {
			perror("mount");
			return 1;
		}
	} else {
		bzero(&hma, sizeof(hma));
		hma.fspec = statfsData.f_mntfromname;
		if(mount("hfs", statfsData.f_mntonname, 0, &hma) < 0) {
			perror("mount");
			return 1;
		}
	}

	return 0;
}
