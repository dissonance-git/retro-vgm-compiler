param(
    [string]$WorkRoot = "",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RetroRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Builder = Join-Path $RetroRoot 'tools\build_private_foobar_components.ps1'
$ProviderSmokeSource = Join-Path $RetroRoot 'tests\integration\snesapu-runtime\snesapu_provider_export_smoke.cpp'

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $RetroRoot '.private-component-build'
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RetroRoot 'dist\private-components'
}

function Need-File([string]$Path, [string]$Label) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label missing: $Path"
    }
}

function Need-Command([string]$Name) {
    if (!(Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command is not on PATH: $Name"
    }
}

Need-File $Builder 'canonical private component builder'
Need-File $ProviderSmokeSource 'SNESAPU provider export smoke source'
Need-Command 'python'
Need-Command '7z'

Write-Host '== Retirement gate 1/3: run the canonical private component build and all embedded audits =='
& $Builder -WorkRoot $WorkRoot -OutputRoot $OutputRoot
if ($LASTEXITCODE -ne 0) {
    throw "canonical private component builder failed with exit code $LASTEXITCODE"
}

$SpcComponent = Join-Path $OutputRoot 'foo_snesapu.private.fb2k-component'
$Bundle = Join-Path $OutputRoot 'private-foobar-vgm-spc.zip'
Need-File $SpcComponent 'packaged SPC component'
Need-File $Bundle 'final private VGM/SPC bundle'

Write-Host '== Retirement gate 2/3: execute the packaged x86 SNESAPU provider ABI =='
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
Need-File $vswhere 'Visual Studio vswhere.exe'
$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsInstall) {
    throw 'Visual Studio C++ x86/x64 tools were not found'
}
$vcvars32 = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvars32.bat'
Need-File $vcvars32 'vcvars32.bat'

$SmokeRoot = Join-Path $WorkRoot 'retirement-snesapu-smoke'
Remove-Item $SmokeRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory $SmokeRoot -Force | Out-Null
& 7z x $SpcComponent "-o$SmokeRoot" -y | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "could not extract packaged SPC component: $SpcComponent"
}

$PackagedSnesapu = Join-Path $SmokeRoot 'SNESAPU.dll'
$PackagedSpcPlayer = Join-Path $SmokeRoot 'spcplayer.exe'
Need-File $PackagedSnesapu 'packaged SNESAPU.dll'
Need-File $PackagedSpcPlayer 'packaged spcplayer.exe'
$SmokeExe = Join-Path $SmokeRoot 'snesapu_provider_export_smoke.exe'
$SmokeCmd = Join-Path $SmokeRoot 'run-snesapu-provider-smoke.cmd'

@"
@echo off
call "$vcvars32"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /MT /W4 /O2 /std:c++17 "$ProviderSmokeSource" /Fe:"$SmokeExe"
if errorlevel 1 exit /b 1
"$SmokeExe" "$PackagedSnesapu"
if errorlevel 1 exit /b 1
exit /b 0
"@ | Set-Content $SmokeCmd -Encoding ASCII

& cmd.exe /d /c $SmokeCmd
if ($LASTEXITCODE -ne 0) {
    throw "packaged SNESAPU provider ABI smoke failed with exit code $LASTEXITCODE"
}
Need-File $SmokeExe 'compiled x86 SNESAPU provider smoke'

Write-Host '== Retirement gate 3/3: re-audit the exact final outer bundle =='
& python (Join-Path $RetroRoot 'tools\verify_private_component_bundle.py') $Bundle
if ($LASTEXITCODE -ne 0) {
    throw "final private bundle verification failed with exit code $LASTEXITCODE"
}

Write-Host ''
Write-Host 'vgmspc retirement execution gate PASSED.'
Write-Host 'The old repository is no longer required by the proven build/package path.'
Write-Host 'This gate does not delete anything automatically.'
