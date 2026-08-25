$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Keep this wrapper as the package-level diagnostic gate. The canonical builder
# resolves the materialized SPC parent under foobar2000/foo_snesapu.
$buildScript = Join-Path $PSScriptRoot 'build_private_foobar_components.ps1'
$logPath = Join-Path (Split-Path $PSScriptRoot -Parent) 'private-foobar-build.log'
Remove-Item -LiteralPath $logPath -Force -ErrorAction SilentlyContinue

$failure = $null
try {
    & $buildScript *>&1 | Tee-Object -FilePath $logPath | Out-Null
} catch {
    $failure = $_
    ($_ | Out-String) | Add-Content -LiteralPath $logPath -Encoding UTF8
}

if ($failure) {
    Write-Host '::group::Focused private Foobar build failure diagnostics'
    if (Test-Path -LiteralPath $logPath -PathType Leaf) {
        $lines = @(Get-Content -LiteralPath $logPath)
        # Prefer actual compiler/linker/build/verifier failures. Legacy NASM
        # emits many benign macro warnings containing source filenames; letting
        # those win the first-match budget can hide the actionable failure.
        $hardPattern = '(?i)(Traceback|AssertionError|fatal error|error C\d{4}|error LNK\d{4}|error MSB\d{4}|LNK\d{4}|MSB\d{4}|unresolved external|undefined reference|\berror:|payload mismatch|package members differ|missing required exports|forbidden private imports|machine mismatch|spcplayer expected|did not reach its own usage path|packaged Omniphony)'
        $hardHits = @($lines | Select-String -Pattern $hardPattern -Context 4,6)
        if ($hardHits.Count -gt 0) {
            $hardHits | Select-Object -First 240 | ForEach-Object { Write-Host $_.ToString() }
            Write-Host '--- final captured lines ---'
            $lines | Select-Object -Last 100 | ForEach-Object { Write-Host $_ }
        } else {
            $fallbackPattern = '(?i)(DSP\.asm|APU\.asm|SPC700\.asm|nasm|fatal|undefined|unresolved|syntax)'
            $fallbackHits = @($lines | Select-String -Pattern $fallbackPattern -Context 2,2)
            if ($fallbackHits.Count -gt 0) {
                $fallbackHits | Select-Object -First 120 | ForEach-Object { Write-Host $_.ToString() }
                Write-Host '--- final captured lines ---'
                $lines | Select-Object -Last 100 | ForEach-Object { Write-Host $_ }
            } else {
                Write-Host 'No focused diagnostic pattern matched; showing final 200 captured lines.'
                $lines | Select-Object -Last 200 | ForEach-Object { Write-Host $_ }
            }
        }
    } else {
        Write-Host 'Build failed before a diagnostic log was created.'
    }
    Write-Host '::endgroup::'
    throw $failure
}

Write-Host 'Private Foobar build completed successfully.'
