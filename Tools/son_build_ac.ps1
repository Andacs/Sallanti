# Masaustu kisayolu "Sallanti (son build)" bunu acar: Build\Oyna\Sallanti.exe varsa baslatir; yoksa
# (ilk build hazirlaniyor ya da yeni build yerine konuyor) ne oldugunu soyler. Tools/otomatik_build.sh.
$Kok = Split-Path -Parent $PSScriptRoot
$Exe = Join-Path $Kok 'Build\Oyna\Sallanti.exe'
if (Test-Path $Exe) {
    Start-Process -FilePath $Exe -WorkingDirectory (Split-Path -Parent $Exe)
    exit
}
Add-Type -AssemblyName System.Windows.Forms
$Kilit = Join-Path $Kok 'Saved\otobuild.lock'
$Gunluk = Join-Path $Kok 'Saved\otobuild.log'
$Son = if (Test-Path $Gunluk) { (Get-Content $Gunluk -Tail 1 -Encoding UTF8) } else { '' }
if (Test-Path $Kilit) {
    $Metin = "Build hazırlanıyor, bitince sağ altta bildirim gelecek.`n`nSon durum:`n$Son"
} else {
    $Metin = "Henüz build yok. Bir commit at ya da Git Bash'te çalıştır:`nbash Tools/otomatik_build.sh`n`nSon durum:`n$Son"
}
[System.Windows.Forms.MessageBox]::Show($Metin, 'Sallantı', 'OK', 'Information') | Out-Null
