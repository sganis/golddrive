Write-warning "Checking if WSL feature is installed..."
$i = 0
$installed = $false
while ($i -lt 30) {
  $i +=1  
  $installed = (Get-WindowsOptionalFeature -FeatureName Microsoft-Windows-Subsystem-Linux -Online).State -eq 'Enabled'
  if ($installed) {
    Write-host "WSL feature is installed"
    break
  }
  Write-warning "Retrying in 10 seconds..."
  sleep 10;
}

if (-not $installed) {
    Write-error "WSL feature is not installed"
}

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12


Write-warning "Installing Ubuntu 18.04 for WSL"

(New-Object Net.WebClient).DownloadFile('https://aka.ms/wsl-ubuntu-1804', "$env:TEMP\wsl-ubuntu-1804.zip")
Expand-Archive -Path "$env:TEMP\wsl-ubuntu-1804.zip" -DestinationPath "C:\WSL\Ubuntu1804" -Force
Remove-Item "$env:TEMP\wsl-ubuntu-1804.zip"

$ubuntuExe = "C:\WSL\Ubuntu1804\ubuntu1804.exe"
. $ubuntuExe install --root
. $ubuntuExe run adduser appveyor --gecos `"First,Last,RoomNumber,WorkPhone,HomePhone`" --disabled-password
. $ubuntuExe run "echo 'appveyor:Password12!' | sudo chpasswd"
. $ubuntuExe run usermod -aG sudo appveyor
. $ubuntuExe run "echo -e `"`"appveyor\tALL=(ALL)\tNOPASSWD: ALL`"`" > /etc/sudoers.d/appveyor"
. $ubuntuExe run chmod 0755 /etc/sudoers.d/appveyor
. $ubuntuExe config --default-user appveyor
. $ubuntuExe run sudo apt-get update


wslconfig /setdefault ubuntu-18.04
wsl lsb_release -a
