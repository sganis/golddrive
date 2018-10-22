@echo off

for /f %%i in ('git rev-parse HEAD') do set git_hash=%%i
echo updating git commit sha header to %currSha%
echo const char * git_hash = "%git_hash%"; > version.h