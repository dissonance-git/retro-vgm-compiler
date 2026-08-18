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

$token = $null
foreach ($field in @($response.InputFields)) {
    if ($null -eq $field) { continue }
    $nameProperty = $field.PSObject.Properties['name']
    $valueProperty = $field.PSObject.Properties['value']
    if ($nameProperty -and $valueProperty -and [string]$nameProperty.Value -eq 'token') {
        $candidate = [string]$valueProperty.Value
        if (![string]::IsNullOrWhiteSpace($candidate)) {
            $token = $candidate
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace([string]$token)) {
    $tokenPatterns = @(
        '(?is)<input\b[^>]*\bname\s*=\s*["'']token["''][^>]*\bvalue\s*=\s*["'']([^"'']+)["'']',
        '(?is)<input\b[^>]*\bvalue\s*=\s*["'']([^"'']+)["''][^>]*\bname\s*=\s*["'']token["'']'
    )
    foreach ($pattern in $tokenPatterns) {
        $match = [regex]::Match([string]$response.Content, $pattern)
        if ($match.Success) {
            $token = [System.Net.WebUtility]::HtmlDecode($match.Groups[1].Value)
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace([string]$token)) {
    throw "uploader.jp bootstrap page did not expose a download token: $DownloadPage"
}

Start-Sleep -Seconds 2
$response = Invoke-WebRequest `
    -Uri $DownloadPage `
    -Method Post `
    -Body @{ token = [string]$token } `
    -WebSession $session `
    -Headers $headers

$href = $null
foreach ($link in @($response.Links)) {
    if ($null -eq $link) { continue }
    $hrefProperty = $link.PSObject.Properties['href']
    if (!$hrefProperty) { continue }
    $candidate = [string]$hrefProperty.Value
    if ($candidate -match 'downloadx') {
        $href = $candidate
        break
    }
}

if ([string]::IsNullOrWhiteSpace([string]$href)) {
    $linkMatch = [regex]::Match(
        [string]$response.Content,
        '(?is)href\s*=\s*["'']([^"'']*downloadx[^"'']*)["'']'
    )
    if ($linkMatch.Success) {
        $href = $linkMatch.Groups[1].Value
    }
}

if ([string]::IsNullOrWhiteSpace([string]$href)) {
    throw "uploader.jp did not return a downloadx link for the verified bootstrap"
}

$href = [System.Net.WebUtility]::HtmlDecode([string]$href)
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
