@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -arch=x64 -host_arch=x64 >/dev/null 2>&1
dumpbin /symbols c:\Dev\golddrive\vendor\openssl\lib\x64\libcrypto.lib
