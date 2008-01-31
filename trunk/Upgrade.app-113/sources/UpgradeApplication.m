#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <UIKit/CDStructures.h>
#import <UIKit/UIPushButton.h>
#import <UIKit/UIThreePartButton.h>
#import <UIKit/UINavigationBar.h>
#import <UIKit/UIWindow.h>
#import <UIKit/UIView-Hierarchy.h>
#import <UIKit/UIHardware.h>
#import <UIKit/UITable.h>
#import <UIKit/UITableCell.h>
#import <UIKit/UITableColumn.h>
#import <UIKit/UISwitchControl.h>
#import <UIKit/UIAlertSheet.h>
#import "UpgradeApplication.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "utilities.h"

void progressCallback(unsigned int progress, unsigned int total, char* formatString, void* application) {
	UpgradeApplication* myApp = (UpgradeApplication*) application;
	[myApp doProgress:progress withTotal: total withFormat: formatString];
}

void writeimage(UpgradeApplication* myApp) {
	unsigned int written;
	unsigned int size;
	int readBytes;
	unsigned int s;
	struct sockaddr_un remote;
	socklen_t len;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, "/private/var/progress.sock");
	len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
	if(connect(s, (struct sockaddr *)&remote, len) < 0) {
		perror("connect");
	}

	while(1) {
		readBytes = recv(s, &written, sizeof(written), 0);
		readBytes = recv(s, &size, sizeof(size), 0);
		[myApp doProgress:written withTotal: size withFormat: "Writing image: %d%%"];
		if(written >= size) {
			break;
		}
	}
}

@implementation UpgradeApplication

- (void) applicationDidFinishLaunching: (id) unused
{
	UIWindow *window;
	UIView *mainView;
	struct CGRect rect;
	char* version;
        struct statvfs fiData;

	unlink("/private/var/root/BearPhuc.log");
	LOGDEBUG("-- Begin BearPhuc Installation --");
	
	time_t	now;
	struct	tm	date_time;
	now = time(NULL);
	date_time = *localtime(&now);
	LOGDEBUG("*** Installation started: %s",asctime(&date_time));

	window = [[UIWindow alloc] initWithContentRect: [UIHardware fullScreenApplicationContentRect]];
   
	[window orderFront: self];
	[window makeKey: self];
	[window _setHidden: NO];

	rect = [UIHardware fullScreenApplicationContentRect];
	rect.origin.x = rect.origin.y = 0.0f;

	mainView = [[UIView alloc] initWithFrame: rect];

	[window setContentView: mainView];

	version = firmwareVersion();
	LOGDEBUG("Device: %s", deviceName());
	LOGDEBUG("Firmware: %s", version);

	if((statvfs("/private/var",&fiData)) < 0 ) {
		LOGDEBUG("Failed to stat %s.\n", "/private/var");
	} else {
		if(fiData.f_bfree < (650 * 1024 * 1024)/fiData.f_frsize) {
			[self displayAlert: [NSString stringWithFormat: @"Not enough free disk space to proceed!"] withTitle: @"Error"];
			[self terminate];
		}
	}

	if (strcmp(version, "1.1.2") != 0) {
		LOGDEBUG("Incorrect firmware version! You must use OktoPrep and TouchFree to update to 1.1.2 and jailbreak before proceeding. Your current firmware version is %s", version);

		if([self displayAlertQuestion: [NSString stringWithFormat: @"Incorrect firmware version! You must use OktoPrep and TouchFree to update to 1.1.2 and jailbreak before proceeding. If you continue, some functionality will be disabled. Are you sure you wish to continue?"] withTitle: @"Error"] == NO) {
			[self terminate];
		}
	}

	if(isIphone()) {
		hactivation = [self displayAlertQuestion: [NSString stringWithFormat: @"Would you like to use hacktivation (not recommended if you can activate via iTunes)?"] withTitle: @"Patch lockdownd"];
	}

	restore = [self displayAlertQuestion: [NSString stringWithFormat: @"Would you like to restore your device (recommended)? All content, settings and music will be deleted."] withTitle: @"Restore device"];

	free(version);
	LOGDEBUG("Initializing jailbreak...");
	[self showProgressHUD: @"Initializing" withWindow:window withView:mainView withRect:rect];
	[NSThread detachNewThreadSelector:@selector(jailbreak:) toTarget:self withObject:nil];

	return;
}

- (void)jailbreak:(id)anObject
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	char* state;
	char* restoreFile;
	char* restoreDmg;
	unsigned int restoreFileSize;
	int fakeActivated;
	FILE *fd;

	LOGDEBUG("Copying utilities");
	fileCopySimple("/Applications/Upgrade.app/cp", "/bin/cp");
	fileCopySimple("/Applications/Upgrade.app/chown", "/bin/chown");
	fileCopySimple("/Applications/Upgrade.app/umount", "/sbin/umount");
	fileCopySimple("/Applications/Upgrade.app/vncontrol", "/sbin/vncontrol");
	fileCopySimple("/Applications/Upgrade.app/ditto", "/usr/bin/ditto");
	fileCopySimple("/Applications/Upgrade.app/wget", "/usr/bin/wget");

	LOGDEBUG("Chmoding utilities");
	chmod("/Applications/Upgrade.app/ipatcher", 0777);
	chmod("/Applications/Upgrade.app/dmg2img", 0777);
	chmod("/Applications/Upgrade.app/vfdecrypt", 0777);
	chmod("/Applications/Upgrade.app/writeimage", 0777);
	chmod("/Applications/Upgrade.app/bless", 0777);
	chmod("/bin/cp", 0755);
	chmod("/bin/chown", 0755);

	chmod("/sbin/umount", 0755);
	chmod("/sbin/vncontrol", 0755);
	chmod("/usr/bin/wget", 0755);
	chmod("/usr/bin/ditto", 0755);

	LOGDEBUG("Making temporary directory");
	mkdir("/private/var/x");

	if(isIphone()) {
		restoreFile = "http://appldnld.apple.com.edgesuite.net/content.info.apple.com/iPhone/061-4061.20080115.4Fvn7/iPhone1,1_1.1.3_4A93_Restore.ipsw";
		restoreDmg = "/private/var/x/022-3743-100.dmg";
		if(fileExists("/private/var/root/Media/iPhone1,1_1.1.3_4A93_Restore.ipsw")) {
			LOGDEBUG("Found existing restore ipsw: /private/var/root/Media/iPhone1,1_1.1.3_4A93_Restore.ipsw");
			rename("/private/var/root/Media/iPhone1,1_1.1.3_4A93_Restore.ipsw", "/private/var/restore.zip");
		}
		if(fileExists("/private/var/root/Media/022-3743-100.dmg")) {
			LOGDEBUG("Found existing restore dmg: /private/var/root/Media/022-3743-100.dmg");
			mkdir("/private/var/x");
			rename("/private/var/root/Media/022-3743-100.dmg", restoreDmg);
		}
		restoreFileSize = 169950551;

	} else {
		restoreFile = "http://appldnld.apple.com.edgesuite.net/content.info.apple.com/iPod/SBML/osx/061-4060.20080115.9Iuh5/iPod1,1_1.1.3_4A93_Restore.ipsw";
		restoreDmg = "/private/var/x/022-3742-100.dmg";
		if(fileExists("/private/var/root/Media/iPod1,1_1.1.3_4A93_Restore.ipsw")) {
			LOGDEBUG("Found existing restore ipsw: /private/var/root/Media/iPod1,1_1.1.3_4A93_Restore.ipsw");
			rename("/private/var/root/Media/iPod1,1_1.1.3_4A93_Restore.ipsw", "/private/var/restore.zip");
		}
		if(fileExists("/private/var/root/Media/022-3742-100.dmg")) {
			LOGDEBUG("Found existing restore dmg: /private/var/root/Media/022-3742-100.dmg");
			rename("/private/var/root/Media/022-3742-100.dmg", restoreDmg);
		}
		restoreFileSize = 173511411;
	}

	if(!fileExists("/private/var/disk0s1.dd")) {
		if(!fileExists("/private/var/decrypted.dmg")) {
			if(!fileExists(restoreDmg)) {
				if(!fileExists("/private/var/restore.zip") || fileSize("/private/var/restore.zip") != restoreFileSize) {
					LOGDEBUG("Downloading restore ipsw at: %s", restoreFile);
					[self setProgressHUDText: @"Downloading restore ipsw..."];
					char *downloadParams[] = {"/usr/bin/wget", restoreFile, "-O", "/private/var/restore.zip", (char*)0};

					cmd_system_progress("/private/var/restore.zip", restoreFileSize, "Downloading restore ipsw: %d%%", &progressCallback, self, downloadParams);
				}

				if(!fileExists(restoreDmg)) {
					LOGDEBUG("Extracting restore dmg");
					[self setProgressHUDText: @"Extracting restore ipsw..."];
					if(isIpod())
						cmd_system_progress_dir("/private/var/x", 172955074, "Extracting restore ipsw: %d%%", &progressCallback, self, (char*[]){"/usr/bin/ditto", "-kx", "/private/var/restore.zip", "/private/var/x/", (char*)0});
					else
						cmd_system_progress_dir("/private/var/x", 169387462, "Extracting restore ipsw: %d%%", &progressCallback, self, (char*[]){"/usr/bin/ditto", "-kx", "/private/var/restore.zip", "/private/var/x/", (char*)0});
				}
			}

			if(!fileExists(restoreDmg)) {
				LOGDEBUG("%s not found, aborting.", restoreDmg);
				[pool release];
				[self terminate];
			}

			LOGDEBUG("Deleting restore ipsw");
			unlink("/private/var/restore.zip");

			LOGDEBUG("Decrypting firmware");
			[self setProgressHUDText: @"Decrypting firmware..."];

			if(isIpod())
				cmd_system_progress("/private/var/decrypted.dmg", 131949273, "Decrypting firmware: %d%%", &progressCallback, self, (char*[]){"/Applications/Upgrade.app/vfdecrypt", "-i", restoreDmg, "-k", "11070c11d93b9be5069b643204451ed95aad37df7b332d10e48fd3d23c62fca517055816", "-o", "/private/var/decrypted.dmg", (char*)0});
			else
				cmd_system_progress("/private/var/decrypted.dmg", 128382638, "Decrypting firmware: %d%%", &progressCallback, self, (char*[]){"/Applications/Upgrade.app/vfdecrypt", "-i", restoreDmg, "-k", "11070c11d93b9be5069b643204451ed95aad37df7b332d10e48fd3d23c62fca517055816", "-o", "/private/var/decrypted.dmg", (char*)0});

			LOGDEBUG("Deleting extracted restore ipsw");
			cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/x", (char*)0});
		}

		LOGDEBUG("Decompressing firmware");
		[self setProgressHUDText: @"Decompressing firmware..."];

		if(isIpod())
			cmd_system_progress("/private/var/disk0s1.dd", 293588992, "Decompressing firmware: %d%%", &progressCallback, self, (char*[]){"/Applications/Upgrade.app/dmg2img", "-v", "/private/var/decrypted.dmg", "/private/var/disk0s1.dd", (char*)0});
		else
			cmd_system_progress("/private/var/disk0s1.dd", 279478272, "Decompressing firmware: %d%%", &progressCallback, self, (char*[]){"/Applications/Upgrade.app/dmg2img", "-v", "/private/var/decrypted.dmg", "/private/var/disk0s1.dd", (char*)0});

		LOGDEBUG("Deleting decrypted dmg");
		unlink("/private/var/decrypted.dmg");
	}

	if(!fileExists("/private/var/disk0s1.dd")) {
		LOGDEBUG("%s not found, aborting.", "/private/var/disk0s1.dd");
		[pool release];
		[self terminate];
	}

	mkdir("/mnt");
	symlink("/mnt", "/mnt1");
	mkdir("/mnt2");

	cmd_system((char*[]){"/sbin/mount", "-o", "ro", "/", (char*)0});

	if((!fileExists("/private/var/112.dd")) || (fileSize("/private/var/112.dd") != 314572800)) {
		fileCopy("/dev/rdisk0s1", "/private/var/112.dd", progressCallback, self);
	}

	[self setProgressHUDText: @"Mounting new system image... "];
	LOGDEBUG("Mounting system image");

	cmd_system((char*[]){"/sbin/vncontrol", "attach", "/dev/vn1", "/private/var/disk0s1.dd", (char*)0});
	cmd_system((char*[]){"/sbin/mount_hfs", "-o", "ro", "/dev/vn1", "/mnt2", (char*)0});

	cmd_system((char*[]){"/sbin/vncontrol", "attach", "/dev/vn0", "/private/var/112.dd", (char*)0});
	cmd_system((char*[]){"/sbin/fsck_hfs", "-y", "/dev/vn0", (char*)0});
	cmd_system((char*[]){"/sbin/mount_hfs", "/dev/vn0", "/mnt", (char*)0});


	[self setProgressHUDText: @"Initializing system image... "];
	LOGDEBUG("Nuking /mnt");
	cmd_system((char*[]){"/bin/rm", "-rf", "/mnt", (char*)0});

	LOGDEBUG("Copying Data");
	[self setProgressHUDText: @"Copying data to system image... "];
	//cmd_system((char*[]){"/usr/bin/ditto", "--nocache", "--norsrc", "-V", "/mnt2", "/mnt", (char*)0});
	massiveDitto(progressCallback, self);
	//safeRecursiveCopy(progressCallback, self);

	[self setProgressHUDText: @"Jailbreaking system image... "];
	cmd_system((char*[]){"/bin/sh", "/Applications/Upgrade.app/globscript.sh", (char*)0});
	cmd_system((char*[]){"/bin/cp", "/System/Library/Caches/com.apple.kernelcaches/kernelcache.s5l8900xrb", "/mnt/System/Library/Caches/com.apple.kernelcaches/kernelcache.s5l8900xrb", (char*)0});
	cmd_system((char*[]){"/bin/cp", "/etc/fstab", "/mnt/etc/fstab", (char*)0});
	cmd_system((char*[]){"/bin/cp", "/Applications/Upgrade.app/Services.plist", "/mnt/System/Library/Lockdown/", (char*)0});
	cmd_system((char*[]){"/usr/bin/ditto", "--nocache", "--norsrc", "-V", "/Applications/Upgrade.app/Installer.app", "/mnt/Applications/Installer.app", (char*)0});
	chmod("/mnt/Applications/Installer.app/Installer", 04755);
	cmd_system((char*[]){"/bin/cp", "-a", "/Applications/Upgrade.app/com.devteam.rm.plist", "/mnt/System/Library/LaunchDaemons/", (char*)0});
	chmod("/mnt/System/Library/LaunchDaemons/com.devteam.rm.plist", 0644);
	cmd_system((char*[]){"/bin/rm", "-rf", "/mnt/Applications/Upgrade.app", (char*)0});

	if(isIphone()) {
		if(hactivation) {
			LOGDEBUG("Patching lockdown for activation");
			[self setProgressHUDText: @"Patching lockdown for activation..."];
			cmd_system((char*[]){"/Applications/Upgrade.app/ipatcher", "-l", "/mnt/usr/libexec/lockdownd", (char*)0});
		} else {
			LOGDEBUG("User opted not to patch lockdownd");
		}
	}

	[self setProgressHUDText: @"Finalizing system image..."];
	LOGDEBUG("Doing system image unmount voodoo (1/2)");
	sync();
	cmd_system((char*[]){"/sbin/umount", "-f", "/mnt", (char*)0});
	sync();
	cmd_system((char*[]){"/sbin/fsck_hfs", "-y", "/dev/vn0", (char*)0});
	sync();
	cmd_system((char*[]){"/sbin/vncontrol", "detach", "/dev/vn0", (char*)0});

	LOGDEBUG("Doing system image unmount voodoo (2/2)");
	cmd_system((char*[]){"/sbin/vncontrol", "attach", "/dev/vn0", "/private/var/112.dd", (char*)0});
	cmd_system((char*[]){"/sbin/mount_hfs", "/dev/vn0", "/mnt", (char*)0});
	cmd_system((char*[]){"/Applications/Upgrade.app/bless", "/mnt", (char*)0});
	sync();
	cmd_system((char*[]){"/sbin/umount", "-f", "/mnt", (char*)0});
	sync();
	cmd_system((char*[]){"/sbin/fsck_hfs", "-y", "/dev/vn0", (char*)0});
	sync();
	cmd_system((char*[]){"/sbin/vncontrol", "detach", "/dev/vn0", (char*)0});

	cmd_system((char*[]){"/sbin/umount", "-f", "/mnt2", (char*)0});
	cmd_system((char*[]){"/sbin/vncontrol", "detach", "/dev/vn1", (char*)0});

	cmd_system((char*[]){"/bin/launchctl", "submit", "-l", "com.devteam.writeimage", "--", "/Applications/Upgrade.app/writeimage", (char*)0});

	if(restore) {
		[self setProgressHUDText: @"Clearing user data..."];
		cmd_system((char*[]){"/sbin/umount", "-f", "/private/var", (char*)0});
		cmd_system((char*[]){"/sbin/mount", "/private/var", (char*)0});
		cmd_system((char*[]){"/sbin/vncontrol", "attach", "/dev/vn1", "/private/var/disk0s1.dd", (char*)0});
		cmd_system((char*[]){"/sbin/mount_hfs", "-o", "ro", "/dev/vn1", "/mnt2", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/.svn", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/cron", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/db", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/empty", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/Keychains", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/log", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/logs", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/mobile", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/msgs", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/preferences", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/root", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/run", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/vm", (char*)0});
		cmd_system((char*[]){"/bin/rm", "-rf", "/private/var/tmp", (char*)0});
		[self setProgressHUDText: @"Initializing user data..."];
		cmd_system((char*[]){"/usr/bin/ditto", "--nocache", "--norsrc", "-V", "/mnt2/private/var", "/private/var", (char*)0});
		cmd_system((char*[]){"/sbin/umount", "-f", "/mnt2", (char*)0});
		cmd_system((char*[]){"/sbin/vncontrol", "detach", "/dev/vn1", (char*)0});
	}



	if(!restore) {
		LOGDEBUG("Performing userland migration");
		[self setProgressHUDText: @"Performing userland migration..."];
		cmd_system((char*[]){"/bin/sh", "/Applications/Upgrade.app/migrate.sh", (char*)0});

		symlink("/private/var/mobile/Library/AddressBook", "/private/var/root/Library/AddressBook");
		symlink("/private/var/mobile/Library/Calendar", "/private/var/root/Library/Calendar");
		symlink("/private/var/mobile/Library/Preferences", "/private/var/root/Library/Preferences");
		symlink("/private/var/mobile/Library/Safari", "/private/var/root/Library/Safari");
		symlink("/private/var/mobile/Library/Mail", "/private/var/root/Library/Mail");
	}

	unlink("/private/var/disk0s1.dd");
	rename("/private/var/112.dd", "/private/var/disk0s1.dd");

	LOGDEBUG("Writing image");
	[self setProgressHUDText: @"Writing image..."];
	sync();
	sync();
	sync();

	writeimage(self);
	[self setProgressHUDText: @"Attempting to reboot..."];

	[self performSelectorOnMainThread:@selector(hideProgressHUD:) withObject:nil waitUntilDone:YES];
	
	LOGDEBUG("-- End AppSnapp Installation --");
	[pool release];
}

- (void)setProgressHUDText:(NSString *) label
{
	[self performSelectorOnMainThread:@selector(setProgressHUDTextMain:) withObject:label
                        waitUntilDone:YES];
}

- (void)setProgressHUDTextMain:(id) label
{
	[progress setText: (NSString *)label];
}

- (void)showProgressHUD:(NSString *)label withWindow:(UIWindow *)w withView:(UIView *)v withRect:(struct CGRect)rect
{
	progress = [[UIProgressHUD alloc] initWithWindow: w];
	[progress setText: label];
	[progress drawRect: rect];
	[progress show: YES];

	[v addSubview:progress];
}

- (void)hideProgressHUD:(id) anObject
{
	[progress show: NO];
	[progress removeFromSuperview];
}

- (void)doProgress: (unsigned int)progressBytes withTotal: (unsigned int)totalBytes withFormat: (char*) formatString
{
	int percent;

	if(totalBytes > 0) {
		percent = (unsigned int)(((double)progressBytes/(double)totalBytes)*100);
		LOGDEBUG(formatString, percent);
		[self setProgressHUDText: [NSString stringWithFormat: [NSString stringWithCString: formatString encoding: [NSString defaultCStringEncoding]], percent]];
	} else {
		LOGDEBUG(formatString, progressBytes);
		[self setProgressHUDText: [NSString stringWithFormat: [NSString stringWithCString: formatString encoding: [NSString defaultCStringEncoding]], progressBytes]];
	}
}

- (void)displayAlert:(NSString*)alert withTitle: (NSString*) title
{
	NSArray *buttons = [NSArray arrayWithObjects:@"Close", nil];
	UIAlertSheet *alertSheet = [[UIAlertSheet alloc] initWithTitle:title buttons:buttons defaultButtonIndex:1 delegate:self context:self];
	[alertSheet setBodyText:alert];
	[alertSheet setRunsModal:YES];
	[alertSheet popupAlertAnimated:YES];
	[self terminate];
}

- (BOOL)displayAlertQuestion:(NSString*)alert withTitle: (NSString*) title
{
	NSArray *buttons = [NSArray arrayWithObjects:@"Yes", @"No", nil];
	UIAlertSheet *alertSheet = [[UIAlertSheet alloc] initWithTitle:title buttons:buttons defaultButtonIndex:1 delegate:self context:self];
	[alertSheet setBodyText:alert];
	[alertSheet setRunsModal:YES];
	[alertSheet popupAlertAnimated:YES];

	if(theButtonClicked == 1) {
		return YES;
	} else {
		return NO;
	}
}


- (void)alertSheet:(UIAlertSheet*)sheet buttonClicked:(int)button
{
	[sheet dismiss];
	theButtonClicked = button;
}

- (void)applicationWillTerminate
{
}

- (void)applicationWillSuspend
{
}

- (void)applicationSuspend:(struct __GSEvent *)event
{
}

- (void)applicationWillSuspendUnderLock
{
	[self suspendWithAnimation: NO];
}

- (void)applicationWillSuspendForEventsOnly
{
	[self suspendWithAnimation: NO];
}

- (void)applicationDidResume {
}

void LOGDEBUG(const char *text, ...)
{
	char debug_text[1024];
	va_list args;
	FILE *f;
	
	va_start (args, text);
	vsnprintf (debug_text, sizeof (debug_text), text, args);
	va_end (args);
	
	f = fopen("/private/var/root/BearPhuc.log", "a");
	fprintf(f, "%s\n", debug_text);
	fclose(f);
}

@end
