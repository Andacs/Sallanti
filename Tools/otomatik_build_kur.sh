#!/usr/bin/env bash
# Otomatik build'i bu bilgisayara kurar (bir kez; tekrar calistirmak zararsiz).
#
#   bash Tools/otomatik_build_kur.sh        # kur
#   bash Tools/otomatik_build_kur.sh kaldir # hook satirlarini sil (kisayol kalir)
#
# - .git/hooks/post-commit ve post-merge'e (Git LFS satirlarinin altina) Tools/otomatik_build_baslat.ps1
#   cagrisini ekler: her commit ve pull'dan sonra Tools/otomatik_build.sh arka planda paketler.
#   Tek seferlik atlamak icin: OTOBUILD=0 git commit ...
# - Masaustune "Sallanti (son build)" kisayolu: Build\Oyna\Sallanti.exe.
# Hook'lar repoyla paylasilmaz (.git/hooks); her ekip uyesi isterse kendisi kurar.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOOKS="$(git -C "$ROOT" rev-parse --git-path hooks)"
case "$HOOKS" in /*|?:*) ;; *) HOOKS="$ROOT/$HOOKS" ;; esac
BAS="# >>> Sallanti otomatik build (Tools/otomatik_build_kur.sh) >>>"
SON="# <<< Sallanti otomatik build <<<"

for H in post-commit post-merge; do
  F="$HOOKS/$H"
  [ -f "$F" ] || printf '#!/bin/sh\n' > "$F"
  # Eski blogu sil, sonra (kur ise) yeniden ekle.
  awk -v b="$BAS" -v e="$SON" '$0 == b { skip = 1; next } $0 == e { skip = 0; next } !skip' "$F" > "$F.tmp" && mv "$F.tmp" "$F"
  if [ "${1:-}" != kaldir ]; then
    {
      echo "$BAS"
      echo '[ "${OTOBUILD:-1}" = 0 ] || powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(git rev-parse --show-toplevel)/Tools/otomatik_build_baslat.ps1" >/dev/null 2>&1 || true'
      echo "$SON"
    } >> "$F"
  fi
  chmod +x "$F"
  echo "$H: $([ "${1:-}" = kaldir ] && echo kaldirildi || echo kuruldu)"
done

if [ "${1:-}" != kaldir ]; then
  powershell.exe -NoProfile -Command "
    \$Masaustu = [Environment]::GetFolderPath('Desktop');
    \$K = (New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path \$Masaustu 'Sallanti (son build).lnk'));
    \$K.TargetPath = '$(cygpath -w "$ROOT/Build/Oyna/Sallanti.exe")';
    \$K.WorkingDirectory = '$(cygpath -w "$ROOT/Build/Oyna")';
    \$K.Description = 'Sallanti: son commitin oynanabilir paketi (Tools/otomatik_build.sh)';
    \$K.Save();
    Write-Output ('kisayol: ' + (Join-Path \$Masaustu 'Sallanti (son build).lnk'))"
fi
