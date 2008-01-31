#!/bin/sh
/bin/mkdir /private/var/xtmp/

/bin/cp /mnt/bin/* /private/var/xtmp/
/bin/cp /bin/* /mnt/bin/
/bin/cp /private/var/xtmp/* /mnt/bin/
/bin/rm /private/var/xtmp/*

/bin/cp /mnt/sbin/* /private/var/xtmp/
/bin/cp /sbin/* /mnt/sbin/
/bin/cp /private/var/xtmp/* /mnt/sbin/
/bin/rm /private/var/xtmp/*

/bin/cp /mnt/usr/libexec/* /private/var/xtmp/
/bin/cp /usr/libexec/* /mnt/usr/libexec/
/bin/cp /private/var/xtmp/* /mnt/usr/libexec/
/bin/rm /private/var/xtmp/*

/bin/cp /mnt/usr/bin/* /private/var/xtmp/
/bin/cp /usr/bin/* /mnt/usr/bin/
/bin/cp /private/var/xtmp/* /mnt/usr/bin/
/bin/rm /private/var/xtmp/*

/bin/cp /mnt/usr/sbin/* /private/var/xtmp/
/bin/cp /usr/sbin/* /mnt/usr/sbin/
/bin/cp /private/var/xtmp/* /mnt/usr/sbin/
/bin/rm /private/var/xtmp/*

/bin/cp /etc/ssh* /mnt/etc/
/bin/cp /Library/LaunchDaemons/* /mnt/Library/LaunchDaemons/

/bin/cp /usr/lib/libarmfp.dylib /mnt/usr/lib/libarmfp.dylib
