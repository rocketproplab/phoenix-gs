@echo off
setlocal
cd /d "%~dp0"
set "PATH=C:\msys64\ucrt64\bin;%PATH%"

echo Compiling Phoenix Ground Station Automated Unit Tests...
g++.exe -std=c++17 -fpermissive -static -static-libgcc -static-libstdc++ -I./mock_arduino -I../../gs ./mock_arduino/mock_globals.cpp ./mock_arduino/mock_w5500.cpp ./unit_tests.cpp -o ./unit_tests.exe -w
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Unit test compilation failed.
    exit /b %ERRORLEVEL%
)

unit_tests.exe
