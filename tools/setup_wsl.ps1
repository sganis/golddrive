# Golddrive
# 04/03/2020, sganis
# Install WSL with Ubuntu, ssh server and support user
# For testing purposes
# Updated to use modern wsl --install approach
#
# Run scripts in powershell:
# set-executionpolicy remotesigned
#
# Uninstall:
# wsl --unregister Ubuntu

Write-host "Installing/Updating WSL..."
try {
    # Install WSL with Ubuntu using the modern method
    wsl --install --no-launch -d Ubuntu 2>&1 | Out-Default
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne -1978335191) {
        Write-host "WSL install command completed with code: $LASTEXITCODE"
    }
} catch {
    Write-host "WSL install attempt: $_"
}

# Wait for WSL to be ready
Write-host "Waiting for WSL to be ready..."
Start-Sleep -Seconds 5

# Check if Ubuntu is installed
$distros = wsl --list --quiet 2>&1
Write-host "Installed distributions: $distros"

# Set Ubuntu as default if it exists
try {
    wsl --set-default Ubuntu 2>&1 | Out-Null
} catch {
    Write-host "Could not set default distribution"
}

Write-host "Configuring Ubuntu..."
# Initialize Ubuntu (first run)
wsl -d Ubuntu -- bash -c "echo 'Ubuntu initialized'" 2>&1 | Out-Default

Write-host "Setting up support user..."
wsl -d Ubuntu -- sudo useradd -m -s /bin/bash -G sudo support 2>&1 | Out-Null
wsl -d Ubuntu -- bash -c "echo 'support:support' | sudo chpasswd" 2>&1 | Out-Null
wsl -d Ubuntu -- sudo bash -c "echo 'support ALL=(ALL) NOPASSWD: ALL' > /etc/sudoers.d/support" 2>&1 | Out-Null
wsl -d Ubuntu -- sudo chmod 0440 /etc/sudoers.d/support 2>&1 | Out-Null

Write-host "Installing and configuring SSH server..."
wsl -d Ubuntu -- sudo apt-get update 2>&1 | Out-Null
wsl -d Ubuntu -- sudo apt-get install -y openssh-server 2>&1 | Out-Null
wsl -d Ubuntu -- sudo service ssh start 2>&1 | Out-Null

# Verify SSH is running
$sshStatus = wsl -d Ubuntu -- sudo service ssh status 2>&1
Write-host "SSH Status: $sshStatus"

Write-host "WSL setup completed successfully"