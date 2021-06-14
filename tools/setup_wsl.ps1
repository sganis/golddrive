# Golddrive
# 04/03/2020, sganis
# Install WSL with ubuntu2004, ssh server and support user
# For testing purposes
# It works for new development machines and also in Appveyor
#
# Run scripts in powershell:
# set-executionpolicy remotesigned
#
# Uninstall:
# wslconfig /u Ubuntu-20.04
# rd /s /q C:\MyWSL\ubuntu2004

Write-host "Checking if WSL feature is installed..."
$installed = (Get-WindowsOptionalFeature -FeatureName Microsoft-Windows-Subsystem-Linux -Online).State -eq 'Enabled'
if ($installed) {
    Write-host "WSL feature is installed"
} else {   
    Write-error "WSL feature is not installed, installing..."
    Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux
}

$exe = "C:\MyWSL\Ubuntu2004\ubuntu2004.exe"
$zip = "C:\cache\wsl-ubuntu-2004.zip"
$tar = "C:\cache\ubuntu2004.tar"

New-Item -ItemType Directory -Force -Path C:\MyWSL

if (!(Test-Path $tar)) {
    if (!(Test-Path $exe)) {
        Write-host "Installing Ubuntu 20.04 for WSL"
        if (!(Test-Path $zip)) {
            Write-host "Downloading..."
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            (New-Object Net.WebClient).DownloadFile('https://aka.ms/wslubuntu2004', "$zip")
        } else {
            Write-host "Downloaded already, found in cache..."
        }
        Write-host "Installing..."
        Expand-Archive -Path "$zip" -DestinationPath "C:\MyWSL\ubuntu2004" -Force
        # Remove-Item "$env:TEMP\wsl-ubuntu-2004.zip"
        . $exe install --root
    }

    . $exe run sudo adduser support --gecos `"First,Last,RoomNumber,WorkPhone,HomePhone`" --disabled-password
    . $exe run sudo "echo 'support:support' | sudo chpasswd"
    . $exe run sudo usermod -aG sudo support
    . $exe run sudo "echo -e `"`"support\tALL=(ALL)\tNOPASSWD: ALL`"`" > /etc/sudoers.d/support 2>/dev/null"
    . $exe run chmod 0755 /etc/sudoers.d/support


    # Write-host "Updating..."
    # . $exe run sudo apt-get update
    # Write-host "Checing user..."
    # . $exe run whoami

    Write-host "Installing ssh..."
    . $exe run sudo apt-get remove -y -qq --purge openssh-server `>`/dev/null
    . $exe run sudo apt-get install -y -qq openssh-server `>`/dev/null
    . $exe run sudo service ssh --full-restart

    # Write-Host "Export only available from Version 10.0.20363.720"
    # $host
    # Write-host "Exporting..."
    # wsl --export Ubuntu-20.04 $tar
} else {
    # Write-host "Found in cache, importing image..."
    # wsl --import Ubuntu-20.04 C:\MyWSL $tar
}

