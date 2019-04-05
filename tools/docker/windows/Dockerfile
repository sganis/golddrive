FROM microsoft/windowsservercore@sha256:c06b4bfaf634215ea194e6005450740f3a230b27c510cf8facab1e9c678f3a99

# Install Powershell
ADD https://github.com/PowerShell/PowerShell/releases/download/v6.0.0/PowerShell-6.0.0-win-x64.zip c:/powershell.zip
RUN powershell.exe -Command Expand-Archive c:/powershell.zip c:/PS6 ; Remove-Item c:/powershell.zip
RUN C:/PS6/pwsh.EXE -Command C:/PS6/Install-PowerShellRemoting.ps1

# Install SSH
ADD https://github.com/PowerShell/Win32-OpenSSH/releases/download/0.0.24.0/OpenSSH-Win64.zip c:/openssh.zip
RUN c:/PS6/pwsh.exe -Command Expand-Archive c:/openssh.zip c:/ ; Remove-Item c:/openssh.zip
RUN c:/PS6/pwsh.exe -Command c:/OpenSSH-Win64/Install-SSHd.ps1

# Configure SSH
COPY sshd_config c:/OpenSSH-Win64/sshd_config
COPY sshd_banner c:/OpenSSH-Win64/sshd_banner
WORKDIR c:/OpenSSH-Win64/
# Don't use powershell as -f paramtere causes problems.
RUN c:/OpenSSH-Win64/ssh-keygen.exe -t dsa -N "" -f ssh_host_dsa_key && \
    c:/OpenSSH-Win64/ssh-keygen.exe -t rsa -N "" -f ssh_host_rsa_key && \
    c:/OpenSSH-Win64/ssh-keygen.exe -t ecdsa -N "" -f ssh_host_ecdsa_key && \
    c:/OpenSSH-Win64/ssh-keygen.exe -t ed25519 -N "" -f ssh_host_ed25519_key

# Create a user to login, as containeradministrator password is unknown
RUN net USER ssh "Passw0rd" /ADD && net localgroup "Administrators" "ssh" /ADD

# Set PS6 as default shell
RUN C:/PS6/pwsh.EXE -Command \
    New-Item -Path HKLM:\SOFTWARE -Name OpenSSH -Force; \
    New-ItemProperty -Path HKLM:\SOFTWARE\OpenSSH -Name DefaultShell -Value c:\ps6\pwsh.exe -PropertyType string -Force ; 

RUN C:/PS6/pwsh.EXE -Command \
    ./Install-sshd.ps1; \
    ./FixHostFilePermissions.ps1 -Confirm:$false;

EXPOSE 22
# For some reason SSH stops after build. So start it again when container runs.
CMD [ "c:/ps6/pwsh.exe", "-NoExit", "-Command", "Start-Service" ,"sshd" ]