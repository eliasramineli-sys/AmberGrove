@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
if errorlevel 1 (
    echo.
    echo Nao foi possivel compilar. Consulte o README.md.
    pause
    exit /b 1
)
pause
