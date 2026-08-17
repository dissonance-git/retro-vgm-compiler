param(
    [string]$WorkRoot = "",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RetroRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $WorkRoot = Join-Path $RetroRoot '.private-component-build'
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RetroRoot 'dist\private-components'
}

$VgmSpcCommit = '2b7ec8bbd7326eabee3ba39bb91130b9b128e74b'
$SpcPlayCommit = 'fc770e268ecacb4523699e2edc5c0efdf80957d6'
$OmniphonyCommit = '0fabccb165e6d957cefecc6eeb1264467e7406a4'
$RustToolchain = '1.88.0'

$Scaffold = Join-Path $WorkRoot 'vgmspc-scaffold'
$Omniphony = Join-Path $WorkRoot 'omniphony'
$VgmTree = Join-Path $WorkRoot 'vgm-current'
$FrontierBuild = Join-Path $WorkRoot 'frontier-tests'

function Need-Command([string]$Name) {
    if (!(Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command is not on PATH: $Name"
    }
}

function Run([string]$Exe, [string[]]$Args, [string]$WorkingDirectory = '') {
    if ($WorkingDirectory) { Push-Location $WorkingDirectory }
    try {
        & $Exe @Args
        if ($LASTEXITCODE -ne 0) {
            throw "$Exe failed with exit code $LASTEXITCODE: $($Args -join ' ')"
        }
    } finally {
        if ($WorkingDirectory) { Pop-Location }
    }
}

function Junction([string]$Link, [string]$Target) {
    if (!(Test-Path $Target)) { throw "Junction target missing: $Target" }
    if (Test-Path $Link) { Remove-Item $Link -Recurse -Force }
    $parent = Split-Path $Link -Parent
    if (!(Test-Path $parent)) { New-Item -ItemType Directory $parent -Force | Out-Null }
    cmd /c "mklink /J `"$Link`" `"$Target`"" | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Failed to create junction: $Link -> $Target" }
}

function Find-ReleaseFile([string]$Filter, [string]$Root) {
    $found = Get-ChildItem -Recurse -File -Filter $Filter -Path $Root -ErrorAction SilentlyContinue |
        Where-Object {
            $_.FullName -notlike '*\Debug\*' -and
            $_.FullName -notlike '*\CMakeFiles\*' -and
            $_.FullName -notlike '*\dist\*'
        } |
        Select-Object -First 1
    if (!$found) { throw "$Filter not found under $Root" }
    return $found.FullName
}

foreach ($command in @('git', 'python', 'cmake', 'rustup', 'cargo', 'nasm')) {
    Need-Command $command
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path $vswhere)) { throw 'Visual Studio 2022 / vswhere.exe not found' }
$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsInstall) { throw 'Visual Studio C++ x86/x64 tools were not found' }
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) { throw 'MSBuild was not found' }
$vcvars32 = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvars32.bat'
if (!(Test-Path $vcvars32)) { throw "vcvars32.bat not found: $vcvars32" }

Remove-Item $WorkRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $OutputRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory $WorkRoot, $OutputRoot -Force | Out-Null

Write-Host '== 1. Compile the private source-transport frontier tests =='
Run 'cmake' @('-S', (Join-Path $RetroRoot 'tests\private_components'), '-B', $FrontierBuild, '-G', 'Visual Studio 17 2022', '-A', 'x64')
Run 'cmake' @('--build', $FrontierBuild, '--config', 'Release', '--parallel')
Run 'ctest' @('--test-dir', $FrontierBuild, '-C', 'Release', '--output-on-failure')

Write-Host '== 2. Checkout the proven Windows project scaffold and exact SPCPlay source =='
Run 'git' @('clone', '--recurse-submodules', 'https://github.com/dissonance-git/vgmspc.git', $Scaffold)
Run 'git' @('-C', $Scaffold, 'checkout', '--detach', $VgmSpcCommit)
Run 'git' @('-C', $Scaffold, 'submodule', 'update', '--init', '--recursive')
$actualSpc = (& git -C (Join-Path $Scaffold 'third_party\spcplay') rev-parse HEAD).Trim()
if ($actualSpc -ne $SpcPlayCommit) {
    throw "SPCPlay revision drift: expected $SpcPlayCommit, got $actualSpc"
}

Write-Host '== 3. Checkout and validate the Omniphony source renderer =='
Run 'git' @('clone', 'https://github.com/dissonance-git/Omniphony-Headphones.git', $Omniphony)
Run 'git' @('-C', $Omniphony, 'checkout', '--detach', $OmniphonyCommit)
Run 'rustup' @('toolchain', 'install', $RustToolchain, '--profile', 'minimal')
$OmniRenderer = Join-Path $Omniphony 'omniphony-renderer'
Run 'cargo' @("+$RustToolchain", 'test', '-p', 'source_ffi') $OmniRenderer
Run 'cargo' @("+$RustToolchain", 'build', '--profile', 'release-deploy', '-p', 'source_ffi') $OmniRenderer
$OmniDll = Join-Path $OmniRenderer 'target\release-deploy\omniphony_source.dll'
if (!(Test-Path $OmniDll)) { throw "Omniphony source DLL missing: $OmniDll" }

Write-Host '== 4. Patch and build libvgm for exact source observation/replacement =='
$Libvgm = Join-Path $Scaffold 'libvgm-master'
Run 'python' @((Join-Path $RetroRoot 'patches\libvgm\apply_source_capture.py'), $Libvgm)
Run 'cmake' @(
    '-S', $Libvgm,
    '-B', (Join-Path $Libvgm 'build_x64'),
    '-G', 'Visual Studio 17 2022', '-A', 'x64',
    '-DBUILD_SHARED_LIBS=OFF', '-DBUILD_PLAYER=OFF', '-DBUILD_VGM2WAV=OFF',
    '-DCMAKE_CONFIGURATION_TYPES=Release',
    '-DUTIL_CHARCNV_ICONV=OFF', '-DUTIL_CHARCNV_WINAPI=ON'
)
Run 'cmake' @('--build', (Join-Path $Libvgm 'build_x64'), '--config', 'Release', '--parallel')

Write-Host '== 5. Stage the current VGM source geometry into the proven VS project =='
New-Item -ItemType Directory (Join-Path $VgmTree 'components') -Force | Out-Null
Copy-Item (Join-Path $RetroRoot 'model') (Join-Path $VgmTree 'model') -Recurse -Force
Copy-Item (Join-Path $RetroRoot 'components\vgm') (Join-Path $VgmTree 'components\vgm') -Recurse -Force
$VgmComponent = Join-Path $VgmTree 'components\vgm\foo_input_vgm'
Get-ChildItem (Join-Path $Scaffold 'foo_input_vgm') -File | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $VgmComponent $_.Name) -Force
}

# The modern runtime is split into its own translation unit; the historical
# project predates that file, so add exactly one compile item without changing
# any source include paths.
$vgmProject = Join-Path $VgmComponent 'foo_input_vgm.vcxproj'
$projectText = Get-Content $vgmProject -Raw
if ($projectText -notmatch 'input_vgm_shadow\.cpp') {
    $needle = '    <ClCompile Include="src\input_vgm.cpp" />'
    if (!$projectText.Contains($needle)) { throw 'Could not locate input_vgm.cpp project item' }
    $projectText = $projectText.Replace(
        $needle,
        $needle + "`r`n    <ClCompile Include=`"src\input_vgm_shadow.cpp`" />"
    )
    Set-Content $vgmProject $projectText -Encoding UTF8
}

$fb2k = Join-Path $Scaffold 'SDK-2025-03-07\foobar2000'
$sdkRoot = Join-Path $Scaffold 'SDK-2025-03-07'
$vgmBase = Join-Path $VgmTree 'components\vgm'
$componentBase = Join-Path $VgmTree 'components'
Junction (Join-Path $vgmBase 'SDK') (Join-Path $fb2k 'SDK')
Junction (Join-Path $vgmBase 'helpers') (Join-Path $fb2k 'helpers')
Junction (Join-Path $vgmBase 'shared') (Join-Path $fb2k 'shared')
Junction (Join-Path $vgmBase 'foobar2000_component_client') (Join-Path $fb2k 'foobar2000_component_client')
Junction (Join-Path $componentBase 'pfc') (Join-Path $sdkRoot 'pfc')
Junction (Join-Path $componentBase 'libPPUI') (Join-Path $sdkRoot 'libPPUI')
Junction (Join-Path $componentBase 'libvgm') $Libvgm
Junction (Join-Path $componentBase 'WTL') (Join-Path $Scaffold 'WTL')
Junction (Join-Path $componentBase 'zlib') (Join-Path $Libvgm 'libs\include')

Run 'python' @((Join-Path $RetroRoot 'patches\foo_input_vgm\apply_enhanced_component.py'), (Join-Path $VgmComponent 'src'))
Run $msbuild @((Join-Path $VgmComponent 'foo_input_vgm.sln'), '/p:Configuration=Release', '/p:Platform=x64', '/p:PlatformToolset=v143', '/m', '/v:m')

Write-Host '== 6. Patch the exact editable SNESAPU source and private SPC parent =='
$SpcPlay = Join-Path $Scaffold 'third_party\spcplay'
Run 'python' @((Join-Path $RetroRoot 'patches\snesapu\apply_private_snesapu.py'), $SpcPlay)
Run 'python' @((Join-Path $RetroRoot 'patches\snesapu\apply_private_component.py'), (Join-Path $Scaffold 'foo_snesapu'))

# Historical project junctions retained exactly where the pinned foo_snesapu
# solutions expect them.
$SpcRoot = Join-Path $Scaffold 'foo_snesapu'
Junction (Join-Path $SpcRoot 'foobar2000\SDK') (Join-Path $fb2k 'SDK')
Junction (Join-Path $SpcRoot 'foobar2000\helpers') (Join-Path $fb2k 'helpers')
Junction (Join-Path $SpcRoot 'foobar2000\shared') (Join-Path $fb2k 'shared')
Junction (Join-Path $SpcRoot 'foobar2000\foobar2000_component_client') (Join-Path $fb2k 'foobar2000_component_client')
Junction (Join-Path $SpcRoot 'pfc') (Join-Path $sdkRoot 'pfc')
Junction (Join-Path $SpcRoot 'libPPUI') (Join-Path $sdkRoot 'libPPUI')
Junction (Join-Path $SpcRoot 'WTL') (Join-Path $Scaffold 'WTL')

Write-Host '== 7. Build patched 32-bit SNESAPU =='
$SnesapuSource = Join-Path $SpcPlay 'snesapu.dll'
$SnesapuCmd = Join-Path $WorkRoot 'build-snesapu-x86.cmd'
@"
@echo off
call "$vcvars32"
if errorlevel 1 exit /b 1
cd /d "$SnesapuSource"
if not exist Release mkdir Release
for %%U in (APU DSP SPC700) do (
  nasm -f win32 -O2 %%U.asm -o Release\%%U.obj || exit /b 1
)
rc /nologo /fo Release\version.res version.rc || exit /b 1
cl /nologo /Gz /MT /W3 /O2 /Ob0 /D NDEBUG /D _USRDLL /c SNESAPU.cpp /FoRelease\SNESAPU.obj || exit /b 1
link /nologo /dll /machine:I386 /nodefaultlib /def:SNESAPU.def /out:Release\SNESAPU.dll /implib:Release\snesapu.lib Release\SNESAPU.obj Release\version.res Release\APU.obj Release\DSP.obj Release\SPC700.obj || exit /b 1
dumpbin /exports Release\SNESAPU.dll | findstr /c:"SetDSPSourceCapture" >nul || exit /b 1
dumpbin /exports Release\SNESAPU.dll | findstr /c:"GetDSPSourceData" >nul || exit /b 1
exit /b 0
"@ | Set-Content $SnesapuCmd -Encoding ASCII
Run 'cmd.exe' @('/d', '/c', $SnesapuCmd)

$SnesapuLib = Join-Path $SnesapuSource 'Release\snesapu.lib'
$SnesapuDll = Join-Path $SnesapuSource 'Release\SNESAPU.dll'
if (!(Test-Path $SnesapuLib) -or !(Test-Path $SnesapuDll)) { throw 'Patched SNESAPU outputs are missing' }
Copy-Item $SnesapuLib (Join-Path $SpcRoot 'spcplayer\lib\Win32\snesapu.lib') -Force

Write-Host '== 8. Build source-aware spcplayer and x64 foo_snesapu =='
Run $msbuild @((Join-Path $SpcRoot 'spcplayer\spcplayer.sln'), '/p:Configuration=Release', '/p:Platform=x86', '/p:PlatformToolset=v143', '/m', '/v:m')
Run $msbuild @((Join-Path $SpcRoot 'foobar2000\foo_snesapu\foo_snesapu.sln'), '/p:Configuration=Release', '/p:Platform=x64', '/p:PlatformToolset=v143', '/m', '/v:m')

Write-Host '== 9. Package the two private foobar components =='
$FooVgm = Find-ReleaseFile 'foo_input_vgm.dll' $VgmComponent
$FooSpc = Find-ReleaseFile 'foo_snesapu.dll' $SpcRoot
$SpcPlayer = Find-ReleaseFile 'spcplayer.exe' $SpcRoot

$VgmPackage = Join-Path $WorkRoot 'package-vgm'
$SpcPackage = Join-Path $WorkRoot 'package-spc'
New-Item -ItemType Directory $VgmPackage, $SpcPackage -Force | Out-Null
Copy-Item $FooVgm (Join-Path $VgmPackage 'foo_input_vgm.dll') -Force
Copy-Item $OmniDll (Join-Path $VgmPackage 'omniphony_source.dll') -Force
Copy-Item $FooSpc (Join-Path $SpcPackage 'foo_snesapu.dll') -Force
Copy-Item $SpcPlayer (Join-Path $SpcPackage 'spcplayer.exe') -Force
Copy-Item $SnesapuDll (Join-Path $SpcPackage 'SNESAPU.dll') -Force
Copy-Item $OmniDll (Join-Path $SpcPackage 'omniphony_source.dll') -Force

$VgmZip = Join-Path $WorkRoot 'foo_input_vgm.zip'
$SpcZip = Join-Path $WorkRoot 'foo_snesapu.zip'
Compress-Archive -Path (Join-Path $VgmPackage '*') -DestinationPath $VgmZip -CompressionLevel Optimal
Compress-Archive -Path (Join-Path $SpcPackage '*') -DestinationPath $SpcZip -CompressionLevel Optimal
$VgmComponentPackage = Join-Path $OutputRoot 'foo_input_vgm.private.fb2k-component'
$SpcComponentPackage = Join-Path $OutputRoot 'foo_snesapu.private.fb2k-component'
Move-Item $VgmZip $VgmComponentPackage -Force
Move-Item $SpcZip $SpcComponentPackage -Force

$retroCommit = 'unversioned'
try { $retroCommit = (& git -C $RetroRoot rev-parse HEAD).Trim() } catch {}
$manifest = [ordered]@{
    built_utc = [DateTime]::UtcNow.ToString('o')
    retro_vgm_compiler = $retroCommit
    vgmspc_build_scaffold = $VgmSpcCommit
    spcplay = $SpcPlayCommit
    omniphony = $OmniphonyCommit
    rust_toolchain = $RustToolchain
    final_playback_contract_hz = 48000
    packages = @(
        (Split-Path $VgmComponentPackage -Leaf),
        (Split-Path $SpcComponentPackage -Leaf)
    )
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $OutputRoot 'build-manifest.json') -Encoding UTF8

$hashLines = foreach ($file in @($VgmComponentPackage, $SpcComponentPackage)) {
    $hash = Get-FileHash $file -Algorithm SHA256
    "$($hash.Hash.ToLowerInvariant())  $(Split-Path $file -Leaf)"
}
$hashLines | Set-Content (Join-Path $OutputRoot 'SHA256SUMS.txt') -Encoding ASCII

@"
Private foobar2000 builds. Not a public release.

Install by opening:
  $(Split-Path $VgmComponentPackage -Leaf)
  $(Split-Path $SpcComponentPackage -Leaf)

Each component carries its own omniphony_source.dll. The SPC package also carries
its exact x86 spcplayer.exe and patched SNESAPU.dll. Enhanced and Spatial remain
independent controls; failed source/renderer evidence falls back to the ordinary
stereo path.
"@ | Set-Content (Join-Path $OutputRoot 'README.txt') -Encoding UTF8

$Bundle = Join-Path $OutputRoot 'private-foobar-vgm-spc.zip'
Compress-Archive -Path @(
    $VgmComponentPackage,
    $SpcComponentPackage,
    (Join-Path $OutputRoot 'build-manifest.json'),
    (Join-Path $OutputRoot 'SHA256SUMS.txt'),
    (Join-Path $OutputRoot 'README.txt')
) -DestinationPath $Bundle -CompressionLevel Optimal

Write-Host ''
Write-Host 'Private components built successfully:'
Get-ChildItem $OutputRoot | Format-Table Name, Length
Get-FileHash $Bundle -Algorithm SHA256 | Format-List
