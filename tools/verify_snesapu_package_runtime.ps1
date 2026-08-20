param(
    [string]$PackagePath = "",
    [string]$WorkRoot = ""
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RetroRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($PackagePath)) {
    $PackagePath = Join-Path $RetroRoot 'dist\private-components\foo_snesapu.private.fb2k-component'
}
if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $RetroRoot '.private-component-build\snesapu-package-smoke'
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

$ProviderSmokeSource = Join-Path $RetroRoot 'tests\integration\snesapu-runtime\snesapu_provider_export_smoke.cpp'
Need-File $PackagePath 'packaged SPC component'
Need-File $ProviderSmokeSource 'SNESAPU provider runtime smoke source'
Need-Command '7z'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
Need-File $vswhere 'Visual Studio vswhere.exe'
$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsInstall) { throw 'Visual Studio C++ x86/x64 tools were not found' }
$vcvars32 = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvars32.bat'
Need-File $vcvars32 'vcvars32.bat'

Remove-Item $WorkRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory $WorkRoot -Force | Out-Null
& 7z x $PackagePath "-o$WorkRoot" -y | Out-Null
if ($LASTEXITCODE -ne 0) { throw "could not extract packaged SPC component: $PackagePath" }

$PackagedSnesapu = Join-Path $WorkRoot 'SNESAPU.dll'
$PackagedSpcPlayer = Join-Path $WorkRoot 'spcplayer.exe'
Need-File $PackagedSnesapu 'packaged SNESAPU.dll'
Need-File $PackagedSpcPlayer 'packaged spcplayer.exe'

$SmokeExe = Join-Path $WorkRoot 'snesapu_provider_export_smoke.exe'
$SmokeCmd = Join-Path $WorkRoot 'run-snesapu-provider-smoke.cmd'
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
Write-Host 'SNESAPU_PACKAGE_RUNTIME_OK 1'
