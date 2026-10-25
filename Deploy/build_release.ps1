<#
.SYNOPSIS
    Packs Sunstreak for handing out: dist\Sunstreak-<version>-windows-x64.zip and, through WSL, the Linux archive.

.DESCRIPTION
    .\Deploy\build_release.ps1 [-NoBuild] [-NoLinux]

    Builds and tests the Release (build.ps1 -Test), stages the game, this README, the manual (PDF), the licence, the
    credits and the Noto fonts' licence, and an empty music folder (for the player's own music, the station MY MUSIC) in
    work\release and zips them into dist. The Linux archive comes from
    Deploy/build_linux.sh in the WSL distribution Ubuntu-22.04 (GCC 11, runs on glibc 2.35 and newer, the Steam Deck
    included); without WSL it is skipped. GitHub builds the same archives from .github/workflows/release.yml.

    -NoBuild   pack what bin\msvc holds
    -NoLinux   only the Windows archive

.EXAMPLE
    .\Deploy\build_release.ps1
#>
param(
    [switch]$NoBuild,
    [switch]$NoLinux
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$version = (Select-String -Path (Join-Path $root "CMakeLists.txt") -Pattern 'project\( Sunstreak VERSION ([0-9.]+)').Matches[0].Groups[1].Value
Write-Host "Sunstreak $version"

if (-not $NoBuild) {
    & (Join-Path $root "build.ps1") -Test
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
}

# Windows: the exe carries everything (static runtime, generated assets); the music folder waits for the player's own.
$name = "Sunstreak-$version-windows-x64"
$stage = Join-Path $root "work\release\$name"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item (Join-Path $root "bin\msvc\Sunstreak.exe") $stage
Copy-Item (Join-Path $root "README.md") $stage
foreach ($doc in "LICENSE", "docs\manual\Sunstreak-Manual.pdf", "docs\CREDITS.txt", "docs\OFL-Noto.txt") { Copy-Item (Join-Path $root $doc) $stage }
New-Item -ItemType Directory -Force (Join-Path $stage "music") | Out-Null
Set-Content -Path (Join-Path $stage "music\README.txt") -Encoding utf8 `
    -Value "Put your own music here (.ogg, .mp3, .flac, .wav, .xm, .mod): it plays as the radio station MY MUSIC."
$dist = Join-Path $root "dist"
New-Item -ItemType Directory -Force $dist | Out-Null
$zip = Join-Path $dist "$name.zip"
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -Force
Write-Host ("{0}  {1:N1} MB" -f $zip, ((Get-Item $zip).Length / 1MB))

# Linux through WSL: the source is reached as /mnt/<drive>/...
if (-not $NoLinux) {
    if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
        Write-Host "no WSL: the Linux archive is left to GitHub (release.yml)"
    } else {
        $drive = $root.Substring(0, 1).ToLower()
        $wslRoot = "/mnt/$drive" + ($root.Substring(2) -replace '\\', '/')
        & wsl.exe -d Ubuntu-22.04 -- bash "$wslRoot/Deploy/build_linux.sh"
        if ($LASTEXITCODE -ne 0) { throw "Linux build failed" }
        $tgz = Join-Path $dist "Sunstreak-$version-linux-x86_64.tar.gz"
        Write-Host ("{0}  {1:N1} MB" -f $tgz, ((Get-Item $tgz).Length / 1MB))
    }
}
