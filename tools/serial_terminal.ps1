param([string]$Port="COM5", [int]$Baud=9600)
Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class CT2{
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
$bs=[char]92
$path=[string]$bs+[string]$bs+"."+[string]$bs+$Port
$h=[CT2]::CreateFile($path,[uint32]3221225472,[uint32]0,[IntPtr]::Zero,[uint32]3,[uint32]0,[IntPtr]::Zero)
if ($h -eq [IntPtr](-1)) { Write-Host ("PORT OPEN FAILED: " + [Runtime.InteropServices.Marshal]::GetLastWin32Error()) -ForegroundColor Red; exit 1 }
$dcb=[Runtime.InteropServices.Marshal]::AllocHGlobal(128)
[CT2]::BuildCommDCB("$Baud,n,8,1",$dcb) | Out-Null
if (-not [CT2]::SetCommState($h,$dcb)) {
  Write-Host ("SetCommState failed: " + [Runtime.InteropServices.Marshal]::GetLastWin32Error()) -ForegroundColor Yellow
} else {
  Write-Host "port configured" -ForegroundColor DarkGray
}
$ct=[Runtime.InteropServices.Marshal]::AllocHGlobal(20)
[Runtime.InteropServices.Marshal]::WriteInt32($ct,0,-1); [Runtime.InteropServices.Marshal]::WriteInt32($ct,4,-1)
[Runtime.InteropServices.Marshal]::WriteInt32($ct,8,30); [Runtime.InteropServices.Marshal]::WriteInt32($ct,12,0); [Runtime.InteropServices.Marshal]::WriteInt32($ct,16,0)
[CT2]::SetCommTimeouts($h,$ct) | Out-Null

$buf = New-Object byte[] 1024
function Read-Available([int]$ms) {
  $sb = New-Object System.Text.StringBuilder
  $end = (Get-Date).AddMilliseconds($ms)
  while ((Get-Date) -lt $end) {
    $g = 0
    if ([CT2]::ReadFile($h,$buf,1024,[ref]$g,[IntPtr]::Zero) -and $g -gt 0) {
      [void]$sb.Append([System.Text.Encoding]::ASCII.GetString($buf,0,$g))
    }
  }
  return $sb.ToString()
}

Clear-Host
Write-Host "==============================================================" -ForegroundColor Cyan
Write-Host (" Loongson 1B terminal    " + $Port + " @ " + $Baud + " 8N1") -ForegroundColor Cyan
Write-Host " Commands: ON 3 | OFF | SPEED 200 | START | STOP" -ForegroundColor Cyan
Write-Host " Exit: exit" -ForegroundColor Cyan
Write-Host "==============================================================" -ForegroundColor Cyan
$pre = Read-Available 300
if ($pre) { Write-Host $pre -NoNewline }

while ($true) {
  $cmd = Read-Host
  if ($cmd -eq 'exit') { break }
  $data = [System.Text.Encoding]::ASCII.GetBytes($cmd + "`r`n")
  $w = 0
  [CT2]::WriteFile($h,$data,$data.Length,[ref]$w,[IntPtr]::Zero) | Out-Null
  $resp = Read-Available 700
  if ($resp) { Write-Host $resp -NoNewline -ForegroundColor Green }
}
[CT2]::CloseHandle($h) | Out-Null
