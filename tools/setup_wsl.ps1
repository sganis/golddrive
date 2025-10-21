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

Write-host "Updating WSL..."
try {
    wsl --update 2>&1 | Out-Null
    Write-host "WSL updated successfully"
} catch {
    Write-host "WSL update skipped or failed (may not be needed): $_"
}

Write-host "Checking if WSL feature is installed..."
$result = dism.exe /online /get-featureinfo /featurename:Microsoft-Windows-Subsystem-Linux
$enabled = $result -match "State : Enabled"

if ($enabled) {
    Write-host "WSL feature is already enabled"
} else {   
    Write-host "WSL feature is not enabled, enabling now..."
    Write-host "This may take a few minutes and might require a restart..."
    
    $dismResult = dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
    
    if ($LASTEXITCODE -eq 0) {
        Write-host "WSL feature enabled successfully"
    } elseif ($LASTEXITCODE -eq 3010) {
        Write-host "WSL feature enabled successfully (restart required but skipped)"
    } else {
        Write-error "Failed to enable WSL feature. Exit code: $LASTEXITCODE"
        Write-error "Output: $dismResult"
        exit 1
    }
    
    # Give it a moment to settle
    Start-Sleep -Seconds 5
}

# Verify WSL is working
Write-host "Verifying WSL installation..."
try {
    $wslCheck = wsl --status 2>&1
    Write-host "WSL Status: $wslCheck"
} catch {
    Write-host "WSL status check failed, but continuing..."
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
    Write-host "Running Ubuntu installer..."
    & $exe install --root
    if ($LASTEXITCODE -ne 0) {
        Write-error "Ubuntu installation failed with exit code: $LASTEXITCODE"
        exit 1
    }
}

Write-host "Configuring support user..."
& $exe run sudo adduser support --gecos `"First,Last,RoomNumber,WorkPhone,HomePhone`" --disabled-password
& $exe run sudo "echo 'support:support' | sudo chpasswd"
& $exe run sudo usermod -aG sudo support
& $exe run sudo "echo -e `"`"support\tALL=(ALL)\tNOPASSWD: ALL`"`" > /etc/sudoers.d/support 2>/dev/null"
& $exe run chmod 0755 /etc/sudoers.d/support

Write-host "Installing ssh..."
& $exe run sudo apt-get update
& $exe run sudo apt-get remove -y -qq --purge openssh-server `>`/dev/null
& $exe run sudo apt-get install -y -qq openssh-server `>`/dev/null
& $exe run sudo service ssh --full-restart

Write-host "WSL setup completed successfully"

