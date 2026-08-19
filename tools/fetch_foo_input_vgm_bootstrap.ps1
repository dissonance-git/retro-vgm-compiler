param(
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [Parameter(Mandatory = $true)][string]$DownloadPage,
    [Parameter(Mandatory = $true)][string]$ExpectedSha256
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# The user-supplied foo_input_vgm 0.31 source tree is the canonical bootstrap
# for the private component.  The legacy uploader arguments remain in the
# function signature only so older callers do not break while the build script
# is being simplified.  They are not source authority and are not contacted.
$CanonicalSha256 = 'e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1'
$CanonicalBase64Sha256 = 'e0774dbfe7b8c344adef89814846662dd0c810af03f7c4c4d78b4a43e73af304'
$CanonicalByteLength = 66250
$CanonicalBase64Length = 88336
$CanonicalSourceLabel = 'repository:imports/foo_input_vgm-0.31.zip'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$canonical = Join-Path $repoRoot 'imports\foo_input_vgm-0.31.zip'
$transferParts = @(
    (Join-Path $repoRoot '.delivery-safe\pre00'),
    (Join-Path $repoRoot '.delivery-safe\pre01'),
    (Join-Path $repoRoot '.delivery-safe\pre02'),
    (Join-Path $repoRoot '.delivery-safe\chunk00'),
    (Join-Path $repoRoot '.delivery-safe\chunk01'),
    (Join-Path $repoRoot '.delivery-safe\chunk02'),
    (Join-Path $repoRoot '.delivery-safe\chunk03'),
    (Join-Path $repoRoot '.delivery-safe\chunk04'),
    (Join-Path $repoRoot '.delivery-safe\chunk05a'),
    (Join-Path $repoRoot '.delivery-safe\chunk05b'),
    (Join-Path $repoRoot '.delivery-safe\chunk06'),
    (Join-Path $repoRoot '.delivery-safe\chunk07'),
    (Join-Path $repoRoot '.delivery-safe\chunk08'),
    (Join-Path $repoRoot '.delivery-safe\chunk09'),
    (Join-Path $repoRoot '.delivery-safe\chunk10')
)

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Restore-CanonicalBootstrap {
    if (Test-Path -LiteralPath $canonical -PathType Leaf) {
        $existing = Get-Sha256 $canonical
        if ($existing -eq $CanonicalSha256) {
            return
        }
        Write-Host "repo bootstrap transport needs reconstruction: $existing"
    }

    foreach ($part in $transferParts) {
        if (!(Test-Path -LiteralPath $part -PathType Leaf)) {
            throw "canonical foo_input_vgm 0.31 transfer part missing: $part"
        }
    }

    $b64 = (($transferParts | ForEach-Object { Get-Content -Raw -LiteralPath $_ }) -join '') -replace '\s', ''
    if ($b64.Length -ne $CanonicalBase64Length) {
        throw "foo_input_vgm 0.31 base64 length mismatch: expected $CanonicalBase64Length, got $($b64.Length)"
    }
    $b64Sha = [Convert]::ToHexString(
        [Security.Cryptography.SHA256]::HashData([Text.Encoding]::ASCII.GetBytes($b64))
    ).ToLowerInvariant()
    if ($b64Sha -ne $CanonicalBase64Sha256) {
        throw "foo_input_vgm 0.31 base64 SHA-256 mismatch: expected $CanonicalBase64Sha256, got $b64Sha"
    }

    $bytes = [Convert]::FromBase64String($b64)
    if ($bytes.Length -ne $CanonicalByteLength) {
        throw "foo_input_vgm 0.31 byte length mismatch: expected $CanonicalByteLength, got $($bytes.Length)"
    }
    [IO.File]::WriteAllBytes($canonical, $bytes)

    $restored = Get-Sha256 $canonical
    if ($restored -ne $CanonicalSha256) {
        Remove-Item -LiteralPath $canonical -Force -ErrorAction SilentlyContinue
        throw "foo_input_vgm 0.31 reconstructed SHA-256 mismatch: expected $CanonicalSha256, got $restored"
    }
}

Restore-CanonicalBootstrap

$parent = Split-Path $OutputPath -Parent
if ($parent -and !(Test-Path $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}
Copy-Item -LiteralPath $canonical -Destination $OutputPath -Force

$actual = Get-Sha256 $OutputPath
if ($actual -ne $CanonicalSha256) {
    Remove-Item -LiteralPath $OutputPath -Force -ErrorAction SilentlyContinue
    throw "foo_input_vgm 0.31 bootstrap copy SHA-256 mismatch: expected $CanonicalSha256, got $actual"
}

# Keep the current caller's manifest fields truthful until the small legacy
# parameter surface in build_private_foobar_components.ps1 is removed outright.
Set-Variable -Name VgmBootstrapSha256 -Value $CanonicalSha256 -Scope 1 -ErrorAction SilentlyContinue
Set-Variable -Name VgmBootstrapUrl -Value $CanonicalSourceLabel -Scope 1 -ErrorAction SilentlyContinue

Write-Host "verified foo_input_vgm 0.31 bootstrap: $actual"
