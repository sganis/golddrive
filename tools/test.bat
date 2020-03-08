::appveyor test script
::set GOLDDRIVE_HOST=%my_ip%
::set GOLDDRIVE_PASS=%my_variable%
::set GOLDDRIVE_USER=support
::set GOLDDRIVE_PORT=%my_port%
::vstest.console /logger:Appveyor src\test\bin\Debug\golddrive-test.dll

vstest.console %~dp0..\src\build\Debug\golddrive-test.dll /Settings:%~dp0..\src\test\test.runsettings