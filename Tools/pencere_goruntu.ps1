# Bir surecin ana penceresinin goruntusunu PNG olarak kaydeder (yalnizca o pencere; masaustu ve ustundeki
# baska pencereler kaydedilmez: PrintWindow, PW_RENDERFULLCONTENT). Paketlenmis Shipping oyununu (konsol ve
# log yok) denemek icin: oyun acik mi, ekranda ne var.
#   powershell -File Tools\pencere_goruntu.ps1 -Process <Proje>-Win64-Shipping -Out C:\...\kare.png
# Paketin pencereli sureci <Proje>-Win64-Shipping (<Proje>.exe yalnizca baslatici); editorden -game ile acildiysa UnrealEditor.
param(
    [Parameter(Mandatory = $true)][string]$Process,
    [Parameter(Mandatory = $true)][string]$Out
)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class SLWin {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
}
"@
$p = Get-Process -Name $Process -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { Write-Output "PENCERE_YOK"; exit 2 }
$r = New-Object SLWin+RECT
[void][SLWin]::GetClientRect($p.MainWindowHandle, [ref]$r)
$w = [Math]::Max(1, $r.R - $r.L); $h = [Math]::Max(1, $r.B - $r.T)
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc()
# 1 = PW_CLIENTONLY, 2 = PW_RENDERFULLCONTENT (DirectX pencereleri icin)
$ok = [SLWin]::PrintWindow($p.MainWindowHandle, $dc, 3)
$g.ReleaseHdc($dc)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "OK $ok ${w}x${h} $Out"
