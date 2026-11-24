@echo off
REM Windows (MinGW g++) build script
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude src\main.cpp -o triage.exe
if errorlevel 1 goto fail
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude tests\test_main.cpp -o run_tests.exe
if errorlevel 1 goto fail
echo Build OK.  Run triage.exe to start, run_tests.exe to run the tests.
goto end
:fail
echo Build FAILED.
:end
