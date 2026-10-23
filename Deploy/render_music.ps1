<#
.SYNOPSIS
    Renders tracks of the VRAudio generators (Totality, Parhelion, Skybreak) into MY MUSIC: bin\msvc\music, as OGG.

.DESCRIPTION
    .\Deploy\render_music.ps1 [-Family G:\Tools\VRAudio] [-Minutes 4.5] [-Force]

    One render after the other at BelowNormal priority (the machine stays usable). Each piece is rendered to a WAV in
    work\music, measured (EBU R128) and encoded to OGG Vorbis at -20 LUFS: MY MUSIC plays at 0.8 of the music volume,
    which puts it at the built-in stations' -22 LUFS. Pieces that are already there are skipped unless -Force.

    -Family   where the generators' repositories are (their bin\msvc\*_render.exe)
    -Minutes  length of each piece
    -Force    render everything again

.EXAMPLE
    .\Deploy\render_music.ps1
#>
param(
    [string]$Family = (Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) "VRAudio"),
    [double]$Minutes = 4.5,
    [switch]$Force
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$work = Join-Path $root "work\music"
$out = Join-Path $root "bin\msvc\music"
New-Item -ItemType Directory -Force $work, $out | Out-Null
$ffmpeg = (Get-Command ffmpeg.exe -ErrorAction SilentlyContinue).Source
if (-not $ffmpeg) { $ffmpeg = "C:\Anw\Tools\ffmpeg\bin\ffmpeg.exe" }
if (-not (Test-Path $ffmpeg)) { throw "ffmpeg not found" }

$min = $Minutes.ToString([Globalization.CultureInfo]::InvariantCulture)
# Name, renderer, arguments: techno, trance and big beat suit the road.
$pieces = @(
    @("Totality - Peak 101", "TechnoGenerator\bin\msvc\tot_render.exe", @("--seed", "101", "--form", "peak")),
    @("Totality - Peak 202", "TechnoGenerator\bin\msvc\tot_render.exe", @("--seed", "202", "--form", "peak")),
    @("Totality - Arc 303", "TechnoGenerator\bin\msvc\tot_render.exe", @("--seed", "303", "--form", "arc")),
    @("Totality - Peak 404", "TechnoGenerator\bin\msvc\tot_render.exe", @("--seed", "404", "--form", "peak")),
    @("Parhelion - Uplifting 11", "TranceGenerator\bin\msvc\parh_render.exe", @("--seed", "11", "--style", "Uplifting")),
    @("Parhelion - Progressive 12", "TranceGenerator\bin\msvc\parh_render.exe", @("--seed", "12", "--style", "Progressive")),
    @("Parhelion - Acid 13", "TranceGenerator\bin\msvc\parh_render.exe", @("--seed", "13", "--style", "Acid")),
    @("Parhelion - Dream House 14", "TranceGenerator\bin\msvc\parh_render.exe", @("--seed", "14", "--style", "`"Dream House`"")),
    @("Skybreak - Block 21", "BreakbeatGenerator\bin\msvc\brk_render.exe", @("--seed", "21", "--profile", "Block")),
    @("Skybreak - Riot 22", "BreakbeatGenerator\bin\msvc\brk_render.exe", @("--seed", "22", "--profile", "Riot")),
    @("Skybreak - Stomp 23", "BreakbeatGenerator\bin\msvc\brk_render.exe", @("--seed", "23", "--profile", "Stomp")),
    @("Skybreak - Nu-Skool 24", "BreakbeatGenerator\bin\msvc\brk_render.exe", @("--seed", "24", "--profile", "Nu-Skool"))
)

foreach ($p in $pieces) {
    $name, $exe, $args0 = $p
    $ogg = Join-Path $out "$name.ogg"
    if ((Test-Path $ogg) -and -not $Force) { Write-Host "have   $name"; continue }
    $renderer = Join-Path $Family $exe
    if (-not (Test-Path $renderer)) { Write-Host "skip   $name (no $renderer)"; continue }
    $wav = Join-Path $work "$name.wav"
    $t0 = Get-Date
    if (-not (Test-Path $wav)) {
        $argList = $args0 + @("--minutes", $min, "--out", "`"$wav`"")
        $proc = Start-Process -FilePath $renderer -ArgumentList $argList -WorkingDirectory (Split-Path $renderer) -PassThru -WindowStyle Hidden
        try { $proc.PriorityClass = "BelowNormal" } catch {}
        $proc.WaitForExit()
        if ($proc.ExitCode -ne 0 -or -not (Test-Path $wav)) { Write-Host "FAIL   $name (exit $($proc.ExitCode))"; continue }
    }

    # Loudness of the render, then one linear gain to -20 LUFS (the masters are limited already). ffmpeg reports on
    # stderr, which Windows PowerShell would turn into an error under "Stop".
    $ErrorActionPreference = "Continue"
    $measure = & $ffmpeg -hide_banner -nostats -i $wav -af ebur128 -f null - 2>&1 | Out-String
    $ErrorActionPreference = "Stop"
    $m = [regex]::Matches($measure, 'I:\s+(-?[0-9.]+) LUFS')
    $lufs = [double]::Parse($m[$m.Count - 1].Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    $gain = (-20.0 - $lufs).ToString("0.00", [Globalization.CultureInfo]::InvariantCulture)
    & $ffmpeg -hide_banner -loglevel error -y -i $wav -af "volume=${gain}dB" -c:a libvorbis -q:a 5 -metadata "title=$name" $ogg
    if ($LASTEXITCODE -ne 0) { Write-Host "FAIL   $name (ffmpeg)"; continue }
    Remove-Item $wav
    Remove-Item "$wav.*" -ErrorAction SilentlyContinue   # the renderers' cue sheets
    Write-Host ("done   {0}  {1:N1} LUFS -> -20  ({2:N0} s)" -f $name, $lufs, ((Get-Date) - $t0).TotalSeconds)
}
