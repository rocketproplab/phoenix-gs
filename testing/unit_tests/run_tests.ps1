$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

Write-Host "Compiling Phoenix Ground Station Automated Unit Tests..." -ForegroundColor Cyan
& g++.exe -std=c++17 -fpermissive -static -static-libgcc -static-libstdc++ -I./mock_arduino -I../../gs ./mock_arduino/mock_globals.cpp ./mock_arduino/mock_w5500.cpp ./unit_tests.cpp -o ./unit_tests.exe -w

if ($LASTEXITCODE -eq 0) {
    & .\unit_tests.exe
} else {
    Write-Host "[ERROR] Unit test compilation failed." -ForegroundColor Red
}
