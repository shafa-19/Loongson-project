param(
    [string]$Port = "COM5",
    [int]$Baud    = 9600,
    [string]$Log  = "D:\loongson1B_example\example_10_bomb\board_test_log.txt"
)

# Automated play of the bomb defusal game on the real board.
# It uses only what a human would use: the serial console.

Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class CT3{
 [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Auto)]
 public static extern IntPtr CreateFile(string n, uint a, uint s, IntPtr sec, uint d, uint f, IntPtr t);
 [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Auto)]
 public static extern bool BuildCommDCB(string def, IntPtr dcb);
 [DllImport("kernel32.dll", SetLastError=true)] public static extern bool SetCommState(IntPtr h, IntPtr dcb);
 [DllImport("kernel32.dll", SetLastError=true)] public static extern bool SetCommTimeouts(IntPtr h, IntPtr ct);
 [DllImport("kernel32.dll", SetLastError=true)] public static extern bool ReadFile(IntPtr h, byte[] b, uint n, out uint r, IntPtr o);
 [DllImport("kernel32.dll", SetLastError=true)] public static extern bool WriteFile(IntPtr h, byte[] b, uint n, out uint w, IntPtr o);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
}
"@

$bs   = [char]92
$path = [string]$bs + [string]$bs + "." + [string]$bs + $Port
$h = [CT3]::CreateFile($path, [uint32]3221225472, [uint32]0, [IntPtr]::Zero, [uint32]3, [uint32]0, [IntPtr]::Zero)
if ($h -eq [IntPtr](-1)) { Write-Host "PORT OPEN FAILED" -ForegroundColor Red; exit 1 }

$dcb = [Runtime.InteropServices.Marshal]::AllocHGlobal(128)
[CT3]::BuildCommDCB("$Baud,n,8,1", $dcb) | Out-Null
if (-not [CT3]::SetCommState($h, $dcb)) {
    Write-Host ("SetCommState failed (" + [Runtime.InteropServices.Marshal]::GetLastWin32Error() + ") - continuing") -ForegroundColor Yellow
} else { Write-Host "port configured" -ForegroundColor DarkGray }

$ct = [Runtime.InteropServices.Marshal]::AllocHGlobal(20)
[Runtime.InteropServices.Marshal]::WriteInt32($ct, 0, -1)
[Runtime.InteropServices.Marshal]::WriteInt32($ct, 4, -1)
[Runtime.InteropServices.Marshal]::WriteInt32($ct, 8, 30)
[Runtime.InteropServices.Marshal]::WriteInt32($ct, 12, 0)
[Runtime.InteropServices.Marshal]::WriteInt32($ct, 16, 0)
[CT3]::SetCommTimeouts($h, $ct) | Out-Null

$buf  = New-Object byte[] 2048
$all  = New-Object System.Text.StringBuilder

function ReadFor([int]$ms) {
    $sb  = New-Object System.Text.StringBuilder
    $end = (Get-Date).AddMilliseconds($ms)
    while ((Get-Date) -lt $end) {
        $n = 0
        if ([CT3]::ReadFile($h, $buf, 2048, [ref]$n, [IntPtr]::Zero) -and $n -gt 0) {
            [void]$sb.Append([System.Text.Encoding]::ASCII.GetString($buf, 0, $n))
        }
        Start-Sleep -Milliseconds 20
    }
    return $sb.ToString()
}

function Send([string]$s) {
    $d = [System.Text.Encoding]::ASCII.GetBytes($s)
    $w = 0
    [CT3]::WriteFile($h, $d, $d.Length, [ref]$w, [IntPtr]::Zero) | Out-Null
}

# wait until the game prints its prompt again (or the game is over)
function WaitPrompt([int]$timeoutMs) {
    $local = ""
    $end   = (Get-Date).AddMilliseconds($timeoutMs)
    while ((Get-Date) -lt $end) {
        $c = ReadFor 200
        if ($c) { $local += $c; [void]$all.Append($c) }
        if ($local -match 'B O O M|D E F U S E D') { return "over" }
        if ($local -match '>\s*$') { return "prompt" }
    }
    return "timeout"
}

Write-Host "reading the banner ..." -ForegroundColor Cyan
[void]$all.Append((ReadFor 1500))

Write-Host "starting a game (ENTER) ..." -ForegroundColor Cyan
Send "`r"
[void](WaitPrompt 25000)

for ($stage = 1; $stage -le 5; $stage++) {
    if ($all.ToString() -match 'D E F U S E D|B O O M') { break }

    Send "D`r"                                  # scan the code (costs 15 s)
    [void](WaitPrompt 20000)

    # take the LAST scan line of the transcript (the code changes every stage)
    $code = $null
    $mm = [regex]::Matches($all.ToString(), 'scan: the code is ([1-8](?:-[1-8])*)')
    if ($mm.Count -gt 0) { $code = $mm[$mm.Count - 1].Groups[1].Value }
    if (-not $code) { Write-Host "no code found - stopping" -ForegroundColor Red; break }
    Write-Host ("stage " + $stage + ": code = " + $code) -ForegroundColor Green

    foreach ($d in $code.Split('-')) {
        Send ($d + "`r")
        $r = WaitPrompt 12000
        if ($r -eq "over") { break }
    }
}

Write-Host "final read ..." -ForegroundColor Cyan
[void]$all.Append((ReadFor 4000))
[CT3]::CloseHandle($h) | Out-Null

$text = $all.ToString()
Set-Content -Path $Log -Value $text -Encoding ASCII
Write-Host ""
Write-Host "=============== board transcript ===============" -ForegroundColor Cyan
Write-Host $text
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ("log saved: " + $Log)

if ($text -match 'D E F U S E D') { Write-Host "RESULT: DEFUSED on the board" -ForegroundColor Green; exit 0 }
if ($text -match 'B O O M')       { Write-Host "RESULT: exploded (game ran, play was too slow)" -ForegroundColor Yellow; exit 0 }
Write-Host "RESULT: no verdict (check the log)" -ForegroundColor Red
exit 2
