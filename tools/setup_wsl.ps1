# Golddrive
# 04/03/2020, sganis
# Install WSL with ubuntu1804, ssh server and support user
# For testing purposes
# It works for new development machines and also in Appveyor
#
# Run scripts in powershell:
# set-executionpolicy remotesigned
#
# Uninstall:
# wslconfig /u Ubuntu-18.04

Write-host "Checking if WSL feature is installed..."
$installed = (Get-WindowsOptionalFeature -FeatureName Microsoft-Windows-Subsystem-Linux -Online).State -eq 'Enabled'
if ($installed) {
    Write-host "WSL feature is already installed"
} else {   
    Write-host "WSL feature is not installed, enabling..."
    try {
        # Use dism.exe for more reliable installation without restart
        $result = dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
        if ($LASTEXITCODE -eq 0 -or $LASTEXITCODE -eq 3010) {
            Write-host "WSL feature enabled successfully"
        } else {
            Write-error "Failed to enable WSL feature. Exit code: $LASTEXITCODE"
            exit 1
        }
    } catch {
        Write-error "Error enabling WSL feature: $_"
        exit 1
    }
}

$zip = "C:\cache\ubuntu1804.zip"
$exe = "C:\MyWSL\ubuntu1804\ubuntu1804.exe"
New-Item -ItemType Directory -Force -Path C:\MyWSL
if (!(Test-Path $exe)) {
    Write-host "Installing Ubuntu for WSL"
    if (!(Test-Path $zip)) {
        Write-host "Downloading..."
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        (New-Object Net.WebClient).DownloadFile('https://aka.ms/wsl-ubuntu-1804', "$zip")
    } else {
        Write-host "Downloaded already, found in cache..."
    }
    Write-host "Installing..."
    Expand-Archive -Path "$zip" -DestinationPath "C:\MyWSL\ubuntu1804" -Force
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
. $exe run sudo apt-get update
. $exe run sudo apt-get remove -y -qq --purge openssh-server `>`/dev/null
. $exe run sudo apt-get install -y -qq openssh-server `>`/dev/null
. $exe run sudo service ssh --full-restart
# Write-Host "Export only available from Version 10.0.20363.720"
# $host
# Write-host "Exporting..."
# wsl --export Ubuntu-20.04 $tar
