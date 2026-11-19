param([string]$Ports = "COM3,COM4,COM5", [string]$Bauds = "9600,115200")

# Probe every serial port at both speeds: send ENTER and show what comes back.

Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class CP{
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

$bs = [char]92

foreach ($port in $Ports.Split(',')) {
    foreach ($baud in $Bauds.Split(',')) {
        $path = [string]$bs + [string]$bs + "." + [string]$bs + $port
        $h = [CP]::CreateFile($path, [uint32]3221225472, [uint32]0, [IntPtr]::Zero, [uint32]3, [uint32]0, [IntPtr]::Zero)
        if ($h -eq [IntPtr](-1)) {
            Write-Host ("{0} @ {1,-6} : ACILMIR (basqa proqram tutub?)" -f $port, $baud) -ForegroundColor Red
            continue
        }

        $dcb = [Runtime.InteropServices.Marshal]::AllocHGlobal(128)
        [CP]::BuildCommDCB("$baud,n,8,1", $dcb) | Out-Null
        $cfg = [CP]::SetCommState($h, $dcb)

        $ct = [Runtime.InteropServices.Marshal]::AllocHGlobal(20)
        [Runtime.InteropServices.Marshal]::WriteInt32($ct, 0, -1)
        [Runtime.InteropServices.Marshal]::WriteInt32($ct, 4, -1)
        [Runtime.InteropServices.Marshal]::WriteInt32($ct, 8, 30)
        [Runtime.InteropServices.Marshal]::WriteInt32($ct, 12, 0)
        [Runtime.InteropServices.Marshal]::WriteInt32($ct, 16, 0)
        [CP]::SetCommTimeouts($h, $ct) | Out-Null

        # send ENTER twice, like a human would
        $d = [System.Text.Encoding]::ASCII.GetBytes("`r")
        $w = 0
        [CP]::WriteFile($h, $d, 1, [ref]$w, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Milliseconds 400
        [CP]::WriteFile($h, $d, 1, [ref]$w, [IntPtr]::Zero) | Out-Null

        $sb  = New-Object System.Text.StringBuilder
        $buf = New-Object byte[] 512
        $end = (Get-Date).AddMilliseconds(1200)
        while ((Get-Date) -lt $end) {
            $n = 0
            if ([CP]::ReadFile($h, $buf, 512, [ref]$n, [IntPtr]::Zero) -and $n -gt 0) {
                [void]$sb.Append([System.Text.Encoding]::ASCII.GetString($buf, 0, $n))
            }
            Start-Sleep -Milliseconds 25
        }
        [CP]::CloseHandle($h) | Out-Null

        $txt = $sb.ToString()
        $flag = if ($cfg) { "" } else { " [SetCommState FAILED]" }
        if ($txt.Length -eq 0) {
            Write-Host ("{0} @ {1,-6} : sessiz{2}" -f $port, $baud, $flag) -ForegroundColor DarkGray
        } else {
            $clean = ($txt -replace "[^\x20-\x7E]", ".") -replace "\s+", " "
            Write-Host ("{0} @ {1,-6} : {2} bayt  {3}{4}" -f $port, $baud, $txt.Length, $clean.Substring(0, [Math]::Min(160, $clean.Length)), $flag) -ForegroundColor Green
        }
    }
}
