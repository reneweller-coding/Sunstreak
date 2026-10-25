# Takes the manual's pictures (tools/manual/shots.txt) with the game itself: each line a run of
# build\msvc\Release\Sunstreak.exe, muted and windowed, at BelowNormal priority, three at a time, into
# docs/manual/shots/NAME.png.
#
#   powershell -File tools\manual\make_shots.ps1 [-Only name,name] [-Workers 3] [-Size 1600x900]
param([string[]]$Only = @(), [int]$Workers = 3, [string]$Size = "1600x900")
# With -File a list arrives as one string: split it.
$Only = @($Only | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' })
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$exe = Join-Path $root "build\msvc\Release\Sunstreak.exe"
$out = Join-Path $root "docs\manual\shots"
New-Item -ItemType Directory -Force $out | Out-Null
$queue = [System.Collections.Generic.Queue[object]]::new()
foreach ($line in Get-Content (Join-Path $PSScriptRoot "shots.txt")) {
    if ($line.Trim() -eq "" -or $line.TrimStart().StartsWith("#")) { continue }
    $name, $args = $line -split '\|', 2
    $name = $name.Trim()
    if ($Only.Count -gt 0 -and $Only -notcontains $name) { continue }
    $queue.Enqueue(@($name, $args.Trim()))
}
$running = @()
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($running.Count -lt $Workers -and $queue.Count -gt 0) {
        $name, $text = $queue.Dequeue()
        $file = Join-Path $out "$name.png"
        $argList = @($text -split ' ' | Where-Object { $_ -ne '' }) + @("--size", $Size, "--windowed", "--mute", "--screenshot", $file)
        $p = Start-Process -FilePath $exe -ArgumentList $argList -PassThru -WindowStyle Minimized
        try { $p.PriorityClass = 'BelowNormal' } catch {}
        $running += $p
    }
    Start-Sleep -Milliseconds 400
    $running = @($running | Where-Object { -not $_.HasExited })
}
"pictures in $out"
