# Dot-source this file: . .\scripts\Enter-DevEnvironment.ps1
$ErrorActionPreference = 'Stop'
$starfallVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $starfallVswhere)) { throw 'vswhere was not found. Install Microsoft C++ Build Tools.' }
$starfallVsRoot = & $starfallVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $starfallVsRoot) { throw 'No complete MSVC x64 installation was found.' }
$starfallDevCmd = Join-Path $starfallVsRoot 'Common7\Tools\VsDevCmd.bat'
$starfallEnvironment = & $env:ComSpec /d /c "call `"$starfallDevCmd`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'Visual Studio developer environment initialization failed.' }
foreach ($starfallLine in $starfallEnvironment) {
    if ($starfallLine -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
Get-Command cl.exe, cmake.exe, ninja.exe | Select-Object Name, Source
