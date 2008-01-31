#!/bin/sh
/bin/mkdir /private/var/db/timezone
/bin/mv /private/var/db/localtime /private/var/db/timezone/localtime
/bin/mv /private/var/root/Library/Keychains /private/var/Keychains
/bin/chown -R 64:64 /private/var/Keychains

/bin/mv /private/var/root/Library/Preferences/SystemConfiguration /private/var/preferences/SystemConfiguration
/bin/mv /private/var/root/Library/Preferences/csidata /private/var/preferences/csidata
/bin/mv /private/var/root/Media /private/var/mobile/Media
/bin/rm -rf /private/var/mobile/Library
/bin/mv /private/var/root/Library /private/var/mobile/Library
/bin/mkdir /private/var/root/Library
/bin/mv /private/var/mobile/Library/Lockdown /private/var/root/Library/Lockdown
/bin/chown -R mobile:mobile /private/var/mobile
/bin/chown -R mobile /private/tmp/MediaCache

/bin/mkdir /private/var/logs/Baseband
/bin/mkdir /private/var/logs/AppleSupport
/bin/chmod a+rwx /private/var/logs/Baseband /private/var/logs/AppleSupport

/bin/cp /Applications/Upgrade.app/update-prebinding-paths.txt /private/var/db/dyld/

/bin/rm -rf /private/var/mobile/Library/Installer
