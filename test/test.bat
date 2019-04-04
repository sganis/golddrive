::appveyor test script
set GOLDDRIVE_HOST=95.219.148.103
set GOLDDRIVE_PASS=%my_variable%
set GOLDDRIVE_USER=support
set GOLDDRIVE_PORT=%my_port%
vstest.console /logger:Appveyor src\test\bin\Release\golddrive-test.dll