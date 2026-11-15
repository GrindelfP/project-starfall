param([ValidateSet('dx11', 'dx12')][string]$Backend = 'dx11')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force out/benchmark | Out-Null
foreach ($run in 1..3) {
    $csv = "out/benchmark/$Backend-$run.csv"
    $process = Start-Process -FilePath "$root/out/build/release/starfall.exe" -ArgumentList '--backend', $Backend, '--scene', 'reference', '--benchmark', $csv -PassThru -Wait
    Copy-Item out/build/release/dx11-diagnostics.log "out/benchmark/$Backend-$run.log"
    if ($process.ExitCode -ne 0) { throw "$Backend run $run failed: exit $($process.ExitCode)" }
    Write-Host "$Backend run $run complete: $csv"
}
