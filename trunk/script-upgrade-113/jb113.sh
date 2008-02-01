#!/bin/sh
#
# Script for soft upgrade & jb from 112 to 113
#
# Author planetbeing, iphone dev team
#
# Mods by The Smuggler ( alive since Apple ][+ 1981 and beyong )
#   * added freespace check & more
#

# for faster download, define your own local HTTP & FTP server
#MY_LOCAL_URL="ftp://192.168.1.100/iphone-1.1.3.restore.ipsw"
#MY_LOCAL_URL="http://myserver.domain/iphone-1.1.3.restore.ipsw"

# cleanup
[ -f /private/var/restore.zip ] && rm -f /private/var/restore.zip
[ -d /private/var/x ] && rm -rf /private/var/x
[ -f /private/var/decrypted.dmg ] && rm -f /private/var/decrypted.dmg
[ -f /private/var/disk0s1.dd ] && rm -f /private/var/disk0s1.dd

ROOTFREESPACE=`df -m /private/var | head -2 | tail -1 | awk '{print $4}'`
FREESPACE=`df -m /private/var | tail -1 | awk '{print $4}'`
PLATFORM=`uname -m | sed -e 's/[0-9].*//g'`
BASED_URL="http://appldnld.apple.com.edgesuite.net/content.info.apple.com/"

# environment variables
if [ "${PLATFORM}" = "iPod" ]; then
  MinMB=425; # minimum required space in MB
  DMG="/private/var/x/022-3742-100.dmg"; # image to decrypt
  KEY=11070c11d93b9be5069b643204451ed95aad37df7b332d10e48fd3d23c62fca517055816
  URL="iPod/SBML/osx/061-4060.20080115.9Iuh5/iPod1,1_1.1.3_4A93_Restore.ipsw"
else
  MinMB=425
  DMG="/private/var/x/022-3743-100.dmg"
  KEY=11070c11d93b9be5069b643204451ed95aad37df7b332d10e48fd3d23c62fca517055816
  URL="iPhone/061-4061.20080115.4Fvn7/iPhone1,1_1.1.3_4A93_Restore.ipsw"
fi
if [ "${MY_LOCAL_URL}" ]; then
  RESTORE_IPSW=${MY_LOCAL_URL}
else
  RESTORE_IPSW=${BASED_URL}${URL}
fi

# enough free space?
if [ "${FREESPACE}" -le "${MinMB}" ]; then
  echo "ERROR: total free space ${FREESPACE}MB -- you need at least ${MinMB}MB"
  echo "ERROR: delete songs, videos, photos, etc and start over!"
  exit 1
else
  echo; echo "*** you have enough space to proceed (${FREESPACE}MB)..."
fi


echo
echo "WARNING: we're not responsible for any damage to your iPhone"
echo "WARNING: FOLLOWING STEPS TAKE ABOUT 5 MINUTES TO COMPLETE"
echo "WARNING: MAKE SURE YOUR SETTING IS IN \"AUTO-LOCK = NEVER\""
echo "WARNING: by pressing RETURN you accept the risk"
echo
echo -n ">>> Press RETURN to continue <<<"
read line


#######################
# lets start to work! #
#######################
cd /
cp cp /bin/
cp ditto wget /usr/bin/
cp umount vncontrol /sbin/
cp chown /usr/sbin/
chmod +x /bin/* /usr/bin/* /sbin/* /usr/sbin/* /vfdecrypt /dmg2img /ipatcher


echo "***"; echo "*** fetching the ~160MB restore file"; echo "***"
wget "${RESTORE_IPSW}" -O /private/var/restore.zip
if [ $? -ne 0 ]; then
  echo
  echo "ERROR: could not download restore file using the following URL:"
  echo "       URL=${RESTORE_IPSW}"
  [ -f /private/var/restore.zip ] && rm -f /private/var/restore.zip
  exit 1
fi


echo "***"; echo "*** ditto restore.zip file (~2 minutes)"
ditto -kx /private/var/restore.zip /private/var/x/ > /dev/null
rm -f /private/var/restore.zip
if [ $? -ne 0 ]; then
  echo
  echo "ERROR: ditto can't unzip /private/var/restore.zip"
  [ -d /private/var/x ] && rm -rf /private/var/x
  exit 1
fi


echo "***"; echo "*** vfdecrypt iPod (~2.5 minutes)"
/vfdecrypt -i ${DMG} -k ${KEY} -o /private/var/decrypted.dmg > /dev/null 2>&1
rm -rf /private/var/x
if [ $? -ne 0 ]; then
  echo
  echo "ERROR: vfdecrypt encountered an error"
  [ -f /private/var/decrypted.dmg ] && rm -f /private/var/decrypted.dmg
  exit 1
fi


echo "***"; echo "*** dmg2img (~3 minutes)"; echo "***"
/dmg2img -v /private/var/decrypted.dmg /private/var/disk0s1.dd > /dev/null
rm -f /private/var/decrypted.dmg
if [ $? -ne 0 ]; then
  echo
  echo "ERROR: dmg2img can't convert file"
  [ -f /private/var/disk0s1.dd ] && rm -f /private/var/disk0s1.dd
  exit 1
fi
# make sure the file is indeed there before we start!
if [ ! -f "/private/var/disk0s1.dd" ]; then
  echo "ERROR: /private/var/disk0s1.dd file not found..."
  echo "ERROR: something weird went wrong!"
  exit 1
fi


##\
###| Let's start the work
##/

echo "***"; echo "The hard work starts here (~1 minute)"; echo
# remount
umount -f /private/var ; mount /private/var

# umount and erase anything if /mnt already exists
[ -d /mnt ] && umount -f /mnt > /dev/null 2>&1
[ -d /mnt ] && rm -rf /mnt
[ ! -d /mnt ] && mkdir /mnt

mount -o ro /;
[ $? -ne 0 ] && echo "ERROR: could not remount / in read-only mode" && exit 1

vncontrol attach /dev/vn0 /private/var/disk0s1.dd;
[ $? -ne 0 ] && echo "ERROR: could not attach file to vn0" && exit 1

mount_hfs /dev/vn0 /mnt;
[ $? -ne 0 ] && echo "ERROR: Could not mount the image" && exit 1

cp /System/Library/Caches/com.apple.kernelcaches/kernelcache.s5l8900xrb /mnt/System/Library/Caches/com.apple.kernelcaches/kernelcache.s5l8900xrb
cp /etc/fstab /mnt/etc/fstab
cp -u /bin/* /mnt/bin/
cp -u /sbin/* /mnt/sbin/
cp -u /usr/libexec/* /mnt/usr/libexec/
cp -u /usr/bin/* /mnt/usr/bin/
cp -u /usr/sbin/* /mnt/usr/sbin/
cp /etc/ssh* /mnt/etc/
cp /Services.plist /mnt/System/Library/Lockdown/
cp /usr/lib/libarmfp.dylib /mnt/usr/lib/libarmfp.dylib
cp /Library/LaunchDaemons/* /mnt/Library/LaunchDaemons/

chown root:wheel /Applications/Installer.app/Installer
chmod +s /Applications/Installer.app/Installer
cp -a /Applications/Installer.app /mnt/Applications/

cp -a /com.devteam.rm.plist /mnt/System/Library/LaunchDaemons/
ln -s /private/var/terminfo /mnt/usr/share/terminfo

if [ "${PLATFORM}" = "iPhone" ]; then
  cp /mnt/usr/libexec/lockdownd /mnt/usr/libexec/lockdownd.orig
  /ipatcher -l /mnt/usr/libexec/lockdownd
fi

sync
umount /mnt
sync
fsck_hfs /dev/vn0
sync
vncontrol detach /dev/vn0
sync

vncontrol attach /dev/vn0 /private/var/disk0s1.dd
mount_hfs /dev/vn0 /mnt
sync
umount /mnt
sync
fsck_hfs /dev/vn0
sync
vncontrol detach /dev/vn0
sync

cp -a /usr/share/terminfo /private/var/

[ ! -d /private/var/db/timezone ] && mkdir /private/var/db/timezone
mv /private/var/db/localtime /private/var/db/timezone/localtime
mv /private/var/root/Library/Keychains /private/var/Keychains
chown -R 64:64 /private/var/Keychains

mv /private/var/root/Library/Preferences/SystemConfiguration /private/var/preferences/SystemConfiguration
mv /private/var/root/Library/Preferences/csidata /private/var/preferences/csidata
mv /private/var/root/Media /private/var/mobile/Media
rm -rf /private/var/mobile/Library
mv /private/var/root/Library /private/var/mobile/Library
mkdir /private/var/root/Library
mv /private/var/mobile/Library/Lockdown /private/var/root/Library/Lockdown
chown -R mobile:mobile /private/var/mobile

[ -d /private/tmp/MediaCache ] && chown -R mobile /private/tmp/MediaCache
[ ! -d /private/var/logs/Baseband ] && mkdir /private/var/logs/Baseband

mkdir /private/var/logs/AppleSupport
chmod a+rwx /private/var/logs/Baseband /private/var/logs/AppleSupport

cp /update-prebinding-paths.txt /private/var/db/dyld/

sync
sync
sync

echo "***"; echo "*** Copying image to raw device (~3 minutes left)"
cp /private/var/disk0s1.dd /dev/rdisk0s1
echo "***"; echo "*** Done."; echo "***"
echo -n "*** Sending reboot command, should take a few seconds"; sleep 1
reboot
