# to run scripts in powershell:
# set-executionpolicy remotesigned
Write-host "Checking if WSL feature is installed..."
$i = 0
$installed = $false
while ($i -lt 30) {
  $i +=1  
  $installed = (Get-WindowsOptionalFeature -FeatureName Microsoft-Windows-Subsystem-Linux -Online).State -eq 'Enabled'
  if ($installed) {
    Write-host "WSL feature is installed"
    break
  }
  Write-host "Retrying in 10 seconds..."
  sleep 10;
}

if (-not $installed) {
    Write-error "WSL feature is not installed"
}

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12


Write-host "Installing Ubuntu 18.04 for WSL"

# (New-Object Net.WebClient).DownloadFile('https://aka.ms/wsl-ubuntu-1804', "$env:TEMP\wsl-ubuntu-1804.zip")
# Expand-Archive -Path "$env:TEMP\wsl-ubuntu-1804.zip" -DestinationPath "C:\WSL\Ubuntu1804" -Force
# Remove-Item "$env:TEMP\wsl-ubuntu-1804.zip"

$ubuntuExe = "C:\WSL\Ubuntu1804\ubuntu1804.exe"
. $ubuntuExe install --root
. $ubuntuExe run sudo adduser support --gecos `"First,Last,RoomNumber,WorkPhone,HomePhone`" --disabled-password
. $ubuntuExe run sudo "echo 'support:support' | sudo chpasswd"

Write-host "Updating..."
. $ubuntuExe run sudo apt-get update

Write-host "Checing user..."
. $ubuntuExe run whoami

Write-host "Installing ssh..."
. $ubuntuExe run sudo apt-get remove -y -qq --purge openssh-server `>`/dev/null
. $ubuntuExe run sudo apt-get install -y -qq openssh-server `>`/dev/null
. $ubuntuExe run sudo service ssh --full-restart
