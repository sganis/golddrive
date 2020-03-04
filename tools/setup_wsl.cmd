:: setup wsl
 curl.exe -L -o ubuntu-1804.appx https://aka.ms/wsl-ubuntu-1804
 Add-AppxPackage .\ubuntu-1804.appx
 wsl sudo apt-get remove -y --purge openssh-server
 wsl sudo apt-get install -y openssh-server
 wsl sudo service ssh --full-restart
 wsl sudo adduser --disabled-password --gecos "" support
 wsl sudo usermod --password support support
