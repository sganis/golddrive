@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -arch=x64 -host_arch=x64 >/dev/null 2>&1
echo === ssh.exe ===
dumpbin /dependents C:\Windows\System32\OpenSSH\ssh.exe
echo === ssh-keygen.exe ===
dumpbin /dependents C:\Windows\System32\OpenSSH\ssh-keygen.exe
