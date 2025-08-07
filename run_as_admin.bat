@echo off
:: Get the directory where this batch file is located
cd /d "%~dp0"

:: Check for admin rights
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Running with admin privileges...
    echo Working directory: %cd%
    python "%~dp0battery_saver.py"
) else (
    echo Requesting admin privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
)
pause