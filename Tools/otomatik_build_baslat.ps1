# Git hook'u cagirir: Tools/otomatik_build.sh'i gizli pencerede, commit'i bekletmeden baslatir.
$Kok = (Split-Path -Parent $PSScriptRoot) -replace '\\', '/'
$Bash = Join-Path $env:ProgramFiles 'Git\bin\bash.exe'
Start-Process -FilePath $Bash -ArgumentList @('-l', "`"$Kok/Tools/otomatik_build.sh`"") -WindowStyle Hidden
