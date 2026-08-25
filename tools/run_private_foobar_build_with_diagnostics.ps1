$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

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
        $pattern = '(?i)(DSP\.asm|APU\.asm|SPC700\.asm|nasm|error[: ]|fatal|LNK\d{4}|MSB\d{4}|undefined|unresolved|syntax)'
        $hits = @($lines | Select-String -Pattern $pattern -Context 2,2)
        if ($hits.Count -gt 0) {
            $hits | Select-Object -First 160 | ForEach-Object { Write-Host $_.ToString() }
        } else {
            Write-Host 'No focused diagnostic pattern matched; showing final 120 captured lines.'
            $lines | Select-Object -Last 120 | ForEach-Object { Write-Host $_ }
        }
    } else {
        Write-Host 'Build failed before a diagnostic log was created.'
    }
    Write-Host '::endgroup::'
    throw $failure
}

Write-Host 'Private Foobar build completed successfully.'
