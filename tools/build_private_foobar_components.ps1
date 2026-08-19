param(
    [string]$WorkRoot = "",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RetroRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($WorkRoot)) { $WorkRoot = Join-Path $RetroRoot '.private-component-build' }
if ([string]::IsNullOrWhiteSpace($OutputRoot)) { $OutputRoot = Join-Path $RetroRoot 'dist\private-components' }

$LibvgmCommit = '64e1de284e9a4305c54dd162ee8c33539a9bc0d1'
$WtlCommit = 'd1cd80e9ce76c4d79da4cf556401ad7a970ce46f'
$SpcPlayCommit = 'fc770e268ecacb4523699e2edc5c0efdf80957d6'
$OmniphonyCommit = '819668d1366710d663ae9c810edbcf9b7e923e19'
$RustToolchain = '1.88.0'
$FoobarSdkDate = '2025-03-07'
$FoobarSdkUrl = 'https://www.foobar2000.org/downloads/SDK-2025-03-07.7z'
$VgmBootstrapUrl = 'https://uu.getuploader.com/foobar2000/download/248'
$VgmBootstrapSha256 = '93d71695fdad062dee47aefa3f857683e4a057302d1a069958eecf5dd18c60ff'
$ExpectedSdkProjectBlob = '56b398318d3258da7caf04c8fa0ee405511e9db0'
$ExpectedPfcProjectBlob = 'a03c81c5e0de11bf6b889d2ac86527c4cd54cefc'
$ExpectedLibvgmCmakeBlob = '1f8fb7f99ec45e1d2af12231f624498e6e252732'
$ExpectedWtlAtlappBlob = '4b3fe38d846da65e2f04257dec2bd4c0bb63cf8e'

$Libvgm = Join-Path $WorkRoot 'libvgm'
$Wtl = Join-Path $WorkRoot 'WTL'
$SpcPlay = Join-Path $WorkRoot 'spcplay'
$Omniphony = Join-Path $WorkRoot 'omniphony'
$SdkExtract = Join-Path $WorkRoot 'foobar-sdk'
$VgmTree = Join-Path $WorkRoot 'vgm-current'
$SpcRoot = Join-Path $WorkRoot 'foo-snesapu-current'
$FrontierBuild = Join-Path $WorkRoot 'frontier-tests'
$LibvgmSourceTestBuild = Join-Path $WorkRoot 'libvgm-source-tests'
$VgmOutDir = Join-Path $WorkRoot 'out-vgm-x64'
$SpcPlayerOutDir = Join-Path $WorkRoot 'out-spcplayer-x86'
$SpcComponentOutDir = Join-Path $WorkRoot 'out-spc-x64'
$VgmBootstrap = Join-Path $WorkRoot 'foo_input_vgm_v0.30.7z'

function Need-Command([string]$Name) {
    if (!(Get-Command $Name -ErrorAction SilentlyContinue)) { throw "Required command is not on PATH: $Name" }
}

function Run([string]$Exe, [string[]]$CommandArgs, [string]$WorkingDirectory = '') {
    if ($WorkingDirectory) { Push-Location $WorkingDirectory }
    try {
        & $Exe @CommandArgs
        if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code ${LASTEXITCODE}: $($CommandArgs -join ' ')" }
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

function Require-File([string]$Path, [string]$Label) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "$Label missing: $Path" }
}

function Get-PEMachine([string]$Path) {
    Require-File $Path 'PE image'
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
    $reader = [System.IO.BinaryReader]::new($stream)
    try {
        if ($reader.ReadUInt16() -ne 0x5A4D) { throw "Not an MZ executable: $Path" }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0 -or ($peOffset + 6) -gt $stream.Length) { throw "Invalid PE offset in $Path" }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) { throw "Missing PE signature: $Path" }
        return [int]$reader.ReadUInt16()
    } finally {
        $reader.Dispose()
        $stream.Dispose()
    }
}

function Assert-PEMachine([string]$Path, [int]$ExpectedMachine, [string]$Label) {
    $actual = Get-PEMachine $Path
    if ($actual -ne $ExpectedMachine) {
        throw ("$Label machine mismatch: expected 0x{0:X4}, got 0x{1:X4}: $Path" -f $ExpectedMachine, $actual)
    }
}

function Get-GitBlobSha([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $header = [Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)`0")
    $payload = New-Object byte[] ($header.Length + $bytes.Length)
    [Array]::Copy($header, 0, $payload, 0, $header.Length)
    [Array]::Copy($bytes, 0, $payload, $header.Length, $bytes.Length)
    $sha = [Security.Cryptography.SHA1]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($payload))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}

function Assert-GitBlob([string]$Path, [string]$Expected, [string]$Label) {
    if (!(Test-Path $Path)) { throw "$Label missing: $Path" }
    $actual = Get-GitBlobSha $Path
    if ($actual -ne $Expected) { throw "$Label source drift: expected $Expected, got $actual" }
}

function Clone-Pin([string]$Url, [string]$Path, [string]$Commit) {
    Run 'git' @('clone', '--filter=blob:none', '--no-checkout', $Url, $Path)
    Run 'git' @('-C', $Path, 'checkout', '--detach', $Commit)
    $actual = (& git -C $Path rev-parse HEAD).Trim()
    if ($actual -ne $Commit) { throw "revision drift for ${Path}: expected $Commit, got $actual" }
}

foreach ($command in @('git', 'python', 'cmake', 'ctest', 'rustup', 'cargo', 'nasm', '7z')) { Need-Command $command }

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
New-Item -ItemType Directory $WorkRoot, $OutputRoot, $VgmOutDir, $SpcPlayerOutDir, $SpcComponentOutDir -Force | Out-Null

Write-Host '== 0. Recover and verify the historical foo_input_vgm bootstrap =='
& (Join-Path $RetroRoot 'tools\fetch_foo_input_vgm_bootstrap.ps1') -OutputPath $VgmBootstrap -DownloadPage $VgmBootstrapUrl -ExpectedSha256 $VgmBootstrapSha256
$env:RETRO_VGM_BOOTSTRAP_ARCHIVE = $VgmBootstrap

Write-Host '== 1. Compile the private source-transport frontier tests =='
Run 'cmake' @('-S', (Join-Path $RetroRoot 'tests\private_components'), '-B', $FrontierBuild, '-G', 'Visual Studio 17 2022', '-A', 'x64')
Run 'cmake' @('--build', $FrontierBuild, '--config', 'Release', '--parallel')
Run 'ctest' @('--test-dir', $FrontierBuild, '-C', 'Release', '--output-on-failure')
$retroCommit = (& git -C $RetroRoot rev-parse HEAD).Trim().ToLowerInvariant()
if ($LASTEXITCODE -ne 0 -or $retroCommit -notmatch '^[0-9a-f]{40}$') {
    throw "Could not capture exact Retro VGM Compiler source commit after preflight: $retroCommit"
}

Write-Host '== 2. Reconstruct external build dependencies from immutable public sources =='
Clone-Pin 'https://github.com/ValleyBell/libvgm.git' $Libvgm $LibvgmCommit
Clone-Pin 'https://github.com/Win32-WTL/WTL.git' $Wtl $WtlCommit
Clone-Pin 'https://github.com/dgrfactory/spcplay.git' $SpcPlay $SpcPlayCommit
Assert-GitBlob (Join-Path $Libvgm 'CMakeLists.txt') $ExpectedLibvgmCmakeBlob 'libvgm pin'
Assert-GitBlob (Join-Path $Wtl 'Include\atlapp.h') $ExpectedWtlAtlappBlob 'WTL pin'

$SdkArchive = Join-Path $WorkRoot "SDK-$FoobarSdkDate.7z"
Invoke-WebRequest -Uri $FoobarSdkUrl -OutFile $SdkArchive -UseBasicParsing
New-Item -ItemType Directory $SdkExtract -Force | Out-Null
Run '7z' @('x', $SdkArchive, "-o$SdkExtract", '-y')
$sdkProject = Get-ChildItem -Recurse -File -Filter 'foobar2000_SDK.vcxproj' -Path $SdkExtract | Select-Object -First 1
if (!$sdkProject) { throw 'Official foobar SDK archive did not contain foobar2000_SDK.vcxproj' }
$fb2k = Split-Path (Split-Path $sdkProject.FullName -Parent) -Parent
$sdkRoot = Split-Path $fb2k -Parent
Assert-GitBlob $sdkProject.FullName $ExpectedSdkProjectBlob 'foobar SDK project'
Assert-GitBlob (Join-Path $sdkRoot 'pfc\pfc.vcxproj') $ExpectedPfcProjectBlob 'foobar pfc project'
Require-File (Join-Path $fb2k 'shared\shared-x64.lib') 'foobar SDK shared x64 library'

Write-Host '== 3. Checkout and validate the Omniphony source renderer =='
Clone-Pin 'https://github.com/dissonance-git/Omniphony-Headphones.git' $Omniphony $OmniphonyCommit
Run 'rustup' @('toolchain', 'install', $RustToolchain, '--profile', 'minimal')
$OmniRenderer = Join-Path $Omniphony 'omniphony-renderer'
$OmniSourceFfi = Join-Path $OmniRenderer 'source_ffi\src\lib.rs'
$omniSourceText = [IO.File]::ReadAllText($OmniSourceFfi)
$omniOld = "            Some(extent_retention),`n            absolute_sample,"
$omniNew = "            Some(extent_retention),`n            None,`n            absolute_sample,"
if ($omniSourceText.Split($omniOld).Length -ne 2) { throw 'Omniphony source_ffi presentation-ramp compatibility anchor drifted' }
[IO.File]::WriteAllText($OmniSourceFfi, $omniSourceText.Replace($omniOld, $omniNew))
Run 'cargo' @("+$RustToolchain", 'test', '-p', 'source_ffi') $OmniRenderer
Run 'cargo' @("+$RustToolchain", 'build', '--profile', 'release-deploy', '-p', 'source_ffi') $OmniRenderer
$OmniDll = Join-Path $OmniRenderer 'target\release-deploy\omniphony_source.dll'
Require-File $OmniDll 'Omniphony source DLL'
Assert-PEMachine $OmniDll 0x8664 'Omniphony source DLL'
Run 'python' @((Join-Path $RetroRoot 'tools\verify_omniphony_runtime_abi.py'), $OmniDll)

Write-Host '== 4. Patch and build pinned libvgm for exact source observation/replacement =='
Run 'python' @((Join-Path $RetroRoot 'patches\libvgm\apply_source_capture.py'), $Libvgm)
Run 'cmake' @('-S', $Libvgm, '-B', (Join-Path $Libvgm 'build_x64'), '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DBUILD_SHARED_LIBS=OFF', '-DBUILD_PLAYER=OFF', '-DBUILD_VGM2WAV=OFF', '-DCMAKE_CONFIGURATION_TYPES=Release', '-DUTIL_CHARCNV_ICONV=OFF', '-DUTIL_CHARCNV_WINAPI=ON')
Run 'cmake' @('--build', (Join-Path $Libvgm 'build_x64'), '--config', 'Release', '--parallel')

Write-Host '== 4b. Run the external libvgm source/resampler regression against that patched tree =='
Run 'cmake' @('-S', (Join-Path $RetroRoot 'tests\integration\libvgm-source'), '-B', $LibvgmSourceTestBuild, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DLIBVGM_ROOT=$Libvgm")
Run 'cmake' @('--build', $LibvgmSourceTestBuild, '--config', 'Release', '--parallel')
Run 'ctest' @('--test-dir', $LibvgmSourceTestBuild, '-C', 'Release', '--output-on-failure')

Write-Host '== 5. Materialize and build the VGM component from this repository =='
$VgmSdkRoot = Join-Path $VgmTree 'components\vgm'
New-Item -ItemType Directory $VgmSdkRoot -Force | Out-Null
Copy-Item (Join-Path $RetroRoot 'model') (Join-Path $VgmTree 'model') -Recurse -Force
Run 'python' @((Join-Path $RetroRoot 'tools\materialize_foo_input_vgm.py'), '--sdk-root', $VgmSdkRoot)
$VgmComponent = Join-Path $VgmSdkRoot 'foo_input_vgm'
$vgmProject = Join-Path $VgmComponent 'foo_input_vgm.vcxproj'
$vgmSolution = Join-Path $VgmComponent 'foo_input_vgm.sln'
$vgmBuildTarget = if (Test-Path $vgmSolution) { $vgmSolution } else { $vgmProject }
Require-File (Join-Path $VgmComponent 'Directory.Build.targets') 'VGM project-owned MSBuild overlay'
$vgmBase = $VgmSdkRoot
$componentBase = Split-Path $vgmBase -Parent
Junction (Join-Path $vgmBase 'SDK') (Join-Path $fb2k 'SDK')
Junction (Join-Path $vgmBase 'helpers') (Join-Path $fb2k 'helpers')
Junction (Join-Path $vgmBase 'shared') (Join-Path $fb2k 'shared')
Junction (Join-Path $vgmBase 'foobar2000_component_client') (Join-Path $fb2k 'foobar2000_component_client')
Junction (Join-Path $componentBase 'pfc') (Join-Path $sdkRoot 'pfc')
Junction (Join-Path $componentBase 'libPPUI') (Join-Path $sdkRoot 'libPPUI')
Junction (Join-Path $componentBase 'libvgm') $Libvgm
Junction (Join-Path $componentBase 'WTL') $Wtl
Junction (Join-Path $componentBase 'zlib') (Join-Path $Libvgm 'libs\include')
$vgmOutArg = '/p:OutDir=' + $VgmOutDir + '\'
Run $msbuild @($vgmBuildTarget, '/p:Configuration=Release', '/p:Platform=x64', '/p:PlatformToolset=v143', $vgmOutArg, '/m', '/v:m')
$FooVgm = Join-Path $VgmOutDir 'foo_input_vgm.dll'
Require-File $FooVgm 'private VGM component'
Assert-PEMachine $FooVgm 0x8664 'private VGM component'

Write-Host '== 6. Materialize the SPC parent/child and patch the pinned editable SNESAPU =='
Run 'python' @((Join-Path $RetroRoot 'tools\materialize_foo_snesapu.py'), $SpcRoot, '--force')
Run 'python' @((Join-Path $RetroRoot 'patches\snesapu\apply_private_snesapu.py'), $SpcPlay)
Junction (Join-Path $SpcRoot 'foobar2000\SDK') (Join-Path $fb2k 'SDK')
Junction (Join-Path $SpcRoot 'foobar2000\helpers') (Join-Path $fb2k 'helpers')
Junction (Join-Path $SpcRoot 'foobar2000\shared') (Join-Path $fb2k 'shared')
Junction (Join-Path $SpcRoot 'foobar2000\foobar2000_component_client') (Join-Path $fb2k 'foobar2000_component_client')
Junction (Join-Path $SpcRoot 'pfc') (Join-Path $sdkRoot 'pfc')
Junction (Join-Path $SpcRoot 'libPPUI') (Join-Path $sdkRoot 'libPPUI')
Junction (Join-Path $SpcRoot 'WTL') $Wtl

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
for %%E in (SetDSPSourceCapture GetDSPSourceData SetDSPPreBrrProvider SetDSPStudioSourceProvider) do (
  dumpbin /exports Release\SNESAPU.dll | findstr /c:"%%E" >nul || exit /b 1
)
exit /b 0
"@ | Set-Content $SnesapuCmd -Encoding ASCII
Run 'cmd.exe' @('/d', '/c', $SnesapuCmd)
$SnesapuLibDir = Join-Path $SnesapuSource 'Release'
$SnesapuDll = Join-Path $SnesapuLibDir 'SNESAPU.dll'
Require-File (Join-Path $SnesapuLibDir 'snesapu.lib') 'patched SNESAPU import library'
Require-File $SnesapuDll 'patched SNESAPU DLL'
Assert-PEMachine $SnesapuDll 0x014C 'patched SNESAPU DLL'

Write-Host '== 8. Build source-aware spcplayer and x64 foo_snesapu =='
$spcPlayerOutArg = '/p:OutDir=' + $SpcPlayerOutDir + '\'
$spcComponentOutArg = '/p:OutDir=' + $SpcComponentOutDir + '\'
Run $msbuild @((Join-Path $SpcRoot 'spcplayer\spcplayer.vcxproj'), '/p:Configuration=Release', '/p:Platform=Win32', '/p:PlatformToolset=v143', "/p:SNESAPUIncludeDir=$SnesapuSource", "/p:SNESAPULibDir=$SnesapuLibDir", $spcPlayerOutArg, '/m', '/v:m')
Run $msbuild @((Join-Path $SpcRoot 'foobar2000\foo_snesapu\foo_snesapu.vcxproj'), '/p:Configuration=Release', '/p:Platform=x64', '/p:PlatformToolset=v143', $spcComponentOutArg, '/m', '/v:m')
$SpcPlayer = Join-Path $SpcPlayerOutDir 'spcplayer.exe'
$FooSpc = Join-Path $SpcComponentOutDir 'foo_snesapu.dll'
Require-File $SpcPlayer 'private SPC child player'
Require-File $FooSpc 'private SPC component'
Assert-PEMachine $SpcPlayer 0x014C 'private SPC child player'
Assert-PEMachine $FooSpc 0x8664 'private SPC component'

Write-Host '== 9. Package and audit the two private foobar components =='
$VgmPackage = Join-Path $WorkRoot 'package-vgm'
$SpcPackage = Join-Path $WorkRoot 'package-spc'
New-Item -ItemType Directory $VgmPackage, $SpcPackage -Force | Out-Null
Copy-Item $FooVgm (Join-Path $VgmPackage 'foo_input_vgm.dll') -Force
Copy-Item $OmniDll (Join-Path $VgmPackage 'omniphony_source.dll') -Force
Copy-Item $FooSpc (Join-Path $SpcPackage 'foo_snesapu.dll') -Force
Copy-Item $SpcPlayer (Join-Path $SpcPackage 'spcplayer.exe') -Force
Copy-Item $SnesapuDll (Join-Path $SpcPackage 'SNESAPU.dll') -Force
Copy-Item $OmniDll (Join-Path $SpcPackage 'omniphony_source.dll') -Force

$omniHash = (Get-FileHash $OmniDll -Algorithm SHA256).Hash
foreach ($copy in @((Join-Path $VgmPackage 'omniphony_source.dll'), (Join-Path $SpcPackage 'omniphony_source.dll'))) {
    if ((Get-FileHash $copy -Algorithm SHA256).Hash -ne $omniHash) { throw "Omniphony package copy differs from built DLL: $copy" }
}

$VgmZip = Join-Path $WorkRoot 'foo_input_vgm.zip'
$SpcZip = Join-Path $WorkRoot 'foo_snesapu.zip'
Compress-Archive -Path (Join-Path $VgmPackage '*') -DestinationPath $VgmZip -CompressionLevel Optimal
Compress-Archive -Path (Join-Path $SpcPackage '*') -DestinationPath $SpcZip -CompressionLevel Optimal
$VgmComponentPackage = Join-Path $OutputRoot 'foo_input_vgm.private.fb2k-component'
$SpcComponentPackage = Join-Path $OutputRoot 'foo_snesapu.private.fb2k-component'
Move-Item $VgmZip $VgmComponentPackage -Force
Move-Item $SpcZip $SpcComponentPackage -Force
Run 'python' @((Join-Path $RetroRoot 'tools\verify_private_component_packages.py'), $VgmComponentPackage, $SpcComponentPackage)

Run 'python' @((Join-Path $RetroRoot 'tools\verify_build_source_provenance.py'), $RetroRoot, '--expected-commit', $retroCommit)
$manifest = [ordered]@{
    built_utc = [DateTime]::UtcNow.ToString('o')
    retro_vgm_compiler = $retroCommit
    foobar_sdk = $FoobarSdkDate
    foobar_sdk_source = $FoobarSdkUrl
    wtl = $WtlCommit
    libvgm = $LibvgmCommit
    foo_input_vgm_bootstrap = [ordered]@{ source = $VgmBootstrapUrl; sha256 = $VgmBootstrapSha256 }
    spcplay = $SpcPlayCommit
    omniphony = $OmniphonyCommit
    rust_toolchain = $RustToolchain
    foo_snesapu_parent_provenance = 'dissonance-git/vgmspc@2b7ec8bbd7326eabee3ba39bb91130b9b128e74b (internal bootstrap only; no live dependency)'
    final_playback_contract_hz = 48000
    binary_architecture = [ordered]@{
        foo_input_vgm = 'x64'
        foo_snesapu = 'x64'
        omniphony_source = 'x64'
        spcplayer = 'x86'
        SNESAPU = 'x86'
    }
    packages = @((Split-Path $VgmComponentPackage -Leaf), (Split-Path $SpcComponentPackage -Leaf))
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

Both components use one 48 kHz final playback timeline in every enhanced/Surround
combination. Each component carries its exact omniphony_source.dll. The SPC
package also carries its exact x86 spcplayer.exe and patched SNESAPU.dll.
enhanced and Surround remain independent controls; failed source/renderer evidence
falls back to the ordinary stereo path.
"@ | Set-Content (Join-Path $OutputRoot 'README.txt') -Encoding UTF8

$Bundle = Join-Path $OutputRoot 'private-foobar-vgm-spc.zip'
Compress-Archive -Path @($VgmComponentPackage, $SpcComponentPackage, (Join-Path $OutputRoot 'build-manifest.json'), (Join-Path $OutputRoot 'SHA256SUMS.txt'), (Join-Path $OutputRoot 'README.txt')) -DestinationPath $Bundle -CompressionLevel Optimal
Run 'python' @((Join-Path $RetroRoot 'tools\verify_private_component_bundle.py'), $Bundle)

Write-Host ''
Write-Host 'Private components built and audited successfully:'
Get-ChildItem $OutputRoot | Format-Table Name, Length
Get-FileHash $Bundle -Algorithm SHA256 | Format-List
