@echo off
:: Check for admin rights
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Running with admin privileges...
    python battery_saver.py
) else (
    echo Requesting admin privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
)
pause