param(
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [Parameter(Mandatory = $true)][string]$DownloadPage,
    [Parameter(Mandatory = $true)][string]$ExpectedSha256
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Compatibility wrapper for older callers. The canonical bootstrap is repository
# evidence under imports/bootstrap; network arguments are deliberately ignored.
$CanonicalSha256 = 'e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$materialized = Join-Path $repoRoot 'imports\foo_input_vgm-0.31.zip'
$reconstructor = Join-Path $repoRoot 'tools\reconstruct_vgm031_bootstrap.py'

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

if (!(Test-Path -LiteralPath $materialized -PathType Leaf) -or ((Get-Sha256 $materialized) -ne $CanonicalSha256)) {
    python -B $reconstructor
    if ($LASTEXITCODE -ne 0) { throw 'canonical foo_input_vgm 0.31 reconstruction failed' }
}

if ((Get-Sha256 $materialized) -ne $CanonicalSha256) {
    throw 'canonical foo_input_vgm 0.31 reconstruction produced the wrong SHA-256'
}

$parent = Split-Path $OutputPath -Parent
if ($parent -and !(Test-Path $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}
Copy-Item -LiteralPath $materialized -Destination $OutputPath -Force

$actual = Get-Sha256 $OutputPath
if ($actual -ne $CanonicalSha256) {
    Remove-Item -LiteralPath $OutputPath -Force -ErrorAction SilentlyContinue
    throw "foo_input_vgm 0.31 bootstrap SHA-256 mismatch: expected $CanonicalSha256, got $actual"
}

Set-Variable -Name VgmBootstrapSha256 -Value $CanonicalSha256 -Scope 1 -ErrorAction SilentlyContinue
Set-Variable -Name VgmBootstrapUrl -Value 'repository:imports/bootstrap/foo_input_vgm-0.31.base64-parts' -Scope 1 -ErrorAction SilentlyContinue
Write-Host "verified foo_input_vgm 0.31 bootstrap: $actual"
