param(
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [Parameter(Mandatory = $true)][string]$DownloadPage,
    [Parameter(Mandatory = $true)][string]$ExpectedSha256
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$parent = Split-Path $OutputPath -Parent
if ($parent -and !(Test-Path $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}

$session = [Microsoft.PowerShell.Commands.WebRequestSession]::new()
$headers = @{ 'User-Agent' = 'Mozilla/5.0 Retro-VGM-Compiler/verified-bootstrap' }
$response = Invoke-WebRequest -Uri $DownloadPage -WebSession $session -Headers $headers
$tokenField = @($response.InputFields | Where-Object { $_.name -eq 'token' }) | Select-Object -First 1
if (!$tokenField -or [string]::IsNullOrWhiteSpace([string]$tokenField.value)) {
    throw "uploader.jp bootstrap page did not expose a download token: $DownloadPage"
}

Start-Sleep -Seconds 2
$response = Invoke-WebRequest `
    -Uri $DownloadPage `
    -Method Post `
    -Body @{ token = [string]$tokenField.value } `
    -WebSession $session `
    -Headers $headers
$downloadLink = @($response.Links | Where-Object { [string]$_.href -match 'downloadx' }) | Select-Object -First 1
if (!$downloadLink) {
    throw "uploader.jp did not return a downloadx link for the verified bootstrap"
}

$href = [System.Net.WebUtility]::HtmlDecode([string]$downloadLink.href)
$href = [System.Uri]::UnescapeDataString($href)
$base = [System.Uri]::new($DownloadPage)
$downloadUri = [System.Uri]::new($base, $href)

Start-Sleep -Seconds 2
Invoke-WebRequest -Uri $downloadUri.AbsoluteUri -OutFile $OutputPath -WebSession $session -Headers $headers
if (!(Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
    throw "verified foo_input_vgm bootstrap download did not create $OutputPath"
}
$actual = (Get-FileHash -LiteralPath $OutputPath -Algorithm SHA256).Hash.ToLowerInvariant()
$expected = $ExpectedSha256.ToLowerInvariant()
if ($actual -ne $expected) {
    Remove-Item -LiteralPath $OutputPath -Force -ErrorAction SilentlyContinue
    throw "foo_input_vgm bootstrap SHA-256 mismatch: expected $expected, got $actual"
}
Write-Host "verified foo_input_vgm bootstrap: $actual"
