@echo off
setlocal
set "starfallExe=%~dp0out\build\release\starfall.exe"
if not exist "%starfallExe%" (
    echo Starfall Release build was not found. From the repository root, run in Windows PowerShell:
    echo   powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -NoExit
    echo   . .\scripts\Enter-DevEnvironment.ps1
    echo   cmake --preset release
    echo   cmake --build --preset release
    pause
    exit /b 1
)
start "" "%starfallExe%" --backend dx12 --scene sandbox
