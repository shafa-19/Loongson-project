# run_on_board.ps1 -- load and start a LoongIDE project on the board
# usage: powershell -ExecutionPolicy Bypass -File run_on_board.ps1 <Project> <Dir>
param(
    [string]$Project = "example_7_1",
    [string]$Dir = "D:\loongson1B_example"
)
$ErrorActionPreference = "Stop"
$proj  = Join-Path $Dir $Project
$lxp   = Join-Path $proj ($Project + ".lxp")
$build = Join-Path $proj "Build"
$exe   = Join-Path $build ($Project + ".exe")
$gdb   = "D:\LoongIDE\mips-2015.05\bin\mips-sde-elf-gdb.exe"
$gs    = Join-Path $PSScriptRoot "run_on_board.gdb"

Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;using System.Runtime.InteropServices;
public class UiHelp2{
 [DllImport("user32.dll")] public static extern void SwitchToThisWindow(IntPtr h, bool alt);
 [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h,int x,int y,int w,int t,bool r);
}
"@
function Info($m){ Write-Host "[board] $m" -ForegroundColor Cyan }

if (-not (Test-Path $exe)) { throw "binary yoxdur: $exe  (LoongIDE-de Ctrl+F9 ile yigin)" }

$ide = Get-Process LoongIDE -ErrorAction SilentlyContinue
if (-not $ide) {
    Info "LoongIDE acilir ..."
    Start-Process -FilePath "D:\LoongIDE\LoongIDE.exe" -ArgumentList ('"' + $lxp + '"') -WorkingDirectory "D:\LoongIDE"
    Start-Sleep -Seconds 18
    $ide = Get-Process LoongIDE
}
$h = $ide.MainWindowHandle
[UiHelp2]::MoveWindow($h,0,0,1500,880,$true) | Out-Null
Start-Sleep -Milliseconds 600
[UiHelp2]::SwitchToThisWindow($h,$true)
Start-Sleep -Milliseconds 800

Info "debug sessiyasi basladilir (F9) ..."
[System.Windows.Forms.SendKeys]::SendWait("{F9}")
Start-Sleep -Seconds 22

Get-Process mips-sde-elf-gdb -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 3

Info "$Project.exe yuklenir ve ise salinir ..."
Start-Process -FilePath $gdb -ArgumentList "-q","-x",$gs,$exe -WorkingDirectory $build
Start-Sleep -Seconds 8

Info "hazir - proqram lovhode isleyir."
Info "konsol: PuTTY, lovhonun UART5 portu, 115200 8N1"
