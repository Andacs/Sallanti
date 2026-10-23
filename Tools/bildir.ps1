# Windows bildirimi (sag alt balon). Tools/otomatik_build.sh build hazir / basarisiz olunca cagirir.
param([string]$Baslik = 'Sallanti', [string]$Metin = '')
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$N = New-Object System.Windows.Forms.NotifyIcon
$N.Icon = [System.Drawing.SystemIcons]::Information
$N.Visible = $true
$N.ShowBalloonTip(8000, $Baslik, $Metin, [System.Windows.Forms.ToolTipIcon]::Info)
Start-Sleep -Seconds 9
$N.Dispose()
