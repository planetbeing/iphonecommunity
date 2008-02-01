echo "installing"
cp ./bin/vsu /usr/libexec/vsu
chmod 4755 /usr/libexec/vsu
mv /Applications/Installer.app/Installer /Applications/Installer.app/Installer.real
mv ./bin/exec_wrapper /Applications/Installer.app/Installer
echo "finit"
