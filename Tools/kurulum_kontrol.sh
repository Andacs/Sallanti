#!/usr/bin/env bash
# Yeni bir bilgisayarin ekip projesini derleyip calistirabildigini ve teslim edebildigini DOGRULAR
# (Docs/KURULUM_YENI_BILGISAYAR.md'nin son adimi). Git Bash'ten, repo kokunden; Unreal editoru KAPALI:
#
#   bash Tools/kurulum_kontrol.sh             # hepsi (ilk calismada shader derlemesiyle 30-60 dk)
#   TRAILER=0 bash Tools/kurulum_kontrol.sh   # trailer cekimini atla
#
# Her madde TAMAM / BOZUK / OLCULEMEDI. Sonuc Saved/kurulum_raporu.txt'e yazilir (git'e girmez).
# Hicbir sey push etmez; sonda git status temiz olmali.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
UPROJ="$ROOT/$(ls *.uproject | head -1)"
PROJE="$(basename "$UPROJ" .uproject)"
UE="/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe"
RAPOR="$ROOT/Saved/kurulum_raporu.txt"
mkdir -p "$ROOT/Saved"
BOZUK=0; OLCULEMEDI=0; SATIR=()
tamam() { echo "  [TAMAM] $*"; SATIR+=("TAMAM       $*"); }
bozuk() { echo "  [BOZUK] $*"; SATIR+=("BOZUK       $*"); BOZUK=$((BOZUK+1)); }
olcu()  { echo "  [OLCULEMEDI] $*"; SATIR+=("OLCULEMEDI  $*"); OLCULEMEDI=$((OLCULEMEDI+1)); }
baslik() { echo; echo "== $*"; SATIR+=("" "$*"); }
surum() { "$@" 2>/dev/null | head -1 | tr -d '\r'; }
sure() { local s=$1; echo "$((s/60)) dk $((s%60)) sn"; }

baslik "K1. Araclar"
for a in git gh ffmpeg ffprobe python; do
  B="--version"; case $a in ffmpeg|ffprobe) B="-version";; esac   # ffmpeg tek tire istiyor
  command -v "$a" > /dev/null && tamam "$a: $(surum "$a" $B | cut -c1-60)" || bozuk "$a bulunamadi (kur ya da terminali yeniden ac)"
done
git lfs version > /dev/null 2>&1 && tamam "git lfs: $(surum git lfs version)" || bozuk "git lfs yok: git lfs install"
PYV=$(python -c "import sys;print('%d.%d'%sys.version_info[:2])" 2>/dev/null || echo yok)
[ "$PYV" = "3.12" ] && tamam "python 3.12" || bozuk "python surumu $PYV (3.12 bekleniyor; Store takma adlari kapali mi?)"
python -c "import numpy, scipy, PIL" 2>/dev/null && tamam "numpy, scipy, pillow" || bozuk "python paketleri eksik: python -m pip install numpy scipy pillow"
[ -x /c/Windows/System32/tar.exe ] && tamam "Windows tar.exe (ZIP icin)" || bozuk "Windows tar.exe yok"
[ -f "$UE" ] && tamam "Unreal 5.8: $UE" || bozuk "Unreal 5.8 yok ya da yanlis yerde (C:\\Program Files\\Epic Games\\UE_5.8 olmali)"
UEV=$(python -c "import json;d=json.load(open(r'C:/Program Files/Epic Games/UE_5.8/Engine/Build/Build.version'));print('%d.%d.%d'%(d['MajorVersion'],d['MinorVersion'],d['PatchVersion']))" 2>/dev/null || echo "?")
# 5.8.x hepsi olur (EngineAssociation "5.8"), ama ekipteki bilgisayarlar AYNI yama surumunde olmali:
# yeni yamanin kaydettigi asset eski yamada uyari verebilir. 16 Ekim: Andac 5.8.0, Yigit 5.8.3 -> esitlendi.
case "$UEV" in
  5.8.*) tamam "Unreal surumu $UEV (ekipteki diger bilgisayarlarla ayni yama olmali)";;
  *)     bozuk "Unreal surumu $UEV: 5.8 bekleniyor";;
esac
VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
if [ -x "$VSWHERE" ]; then
  VS=$("$VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property displayName 2>/dev/null | tr -d '\r')
  [ -n "$VS" ] && tamam "Visual Studio + C++ araclari: $VS" || bozuk "Visual Studio var ama C++ araclari (MSVC) yok: Installer'dan 'C++ ile oyun gelistirme' ekle"
else
  bozuk "Visual Studio yok"
fi
SDK=$(ls "/c/Program Files (x86)/Windows Kits/10/Include" 2>/dev/null | grep "^10\." | sort -V | tail -1)
[ -n "$SDK" ] && tamam "Windows SDK $SDK" || bozuk "Windows SDK yok"

baslik "K2. GitHub ve repo"
if gh auth status > /dev/null 2>&1; then tamam "gh girisli: $(gh api user -q .login 2>/dev/null)"; else bozuk "gh girisli degil: gh auth login"; fi
[ -n "$(git config --global user.name)" ] && [ -n "$(git config --global user.email)" ] \
  && tamam "git kimligi: $(git config --global user.name) <$(git config --global user.email)>" \
  || bozuk "git kimligi yok: git config --global user.name / user.email"
git ls-remote origin HEAD > /dev/null 2>&1 && tamam "uzak repoya erisim: $(git remote get-url origin)" || bozuk "uzak repoya erisilemiyor"
LFSDOSYA=$(git lfs ls-files -n | head -1)
if [ -n "$LFSDOSYA" ]; then
  B=$(stat -c %s "$LFSDOSYA" 2>/dev/null || echo 0)
  [ "$B" -gt 1024 ] && tamam "LFS dosyalari inmis ($LFSDOSYA $B bayt)" || bozuk "LFS dosyalari isaretci ($B bayt): git lfs pull"
else
  olcu "repoda LFS dosyasi yok"
fi
if [ -n "${YAZMA_REPO:-}" ]; then
  P=$(gh api "repos/$YAZMA_REPO" -q .permissions.push 2>/dev/null || echo "?")
  [ "$P" = "true" ] && tamam "$YAZMA_REPO'ya yazma izni var" || bozuk "$YAZMA_REPO'ya yazma izni yok ($P): Andac collaborator eklesin, daveti kabul et"
fi

if tasklist //FI "IMAGENAME eq UnrealEditor.exe" 2>/dev/null | grep -qi UnrealEditor; then
  bozuk "Unreal editoru acik: kapat, tekrar calistir"
else
  baslik "K3. C++ derleme (Build/derle.bat)"
  T0=$(date +%s)
  cmd //c "Build\\derle.bat" > "$ROOT/Saved/kurulum_derle.log" 2>&1
  T1=$(date +%s)
  grep -q "Result: Succeeded" "$ROOT/Saved/kurulum_derle.log" && tamam "derleme $(sure $((T1-T0)))" \
    || { bozuk "derleme basarisiz: Saved/kurulum_derle.log"; grep -m5 -i "error" "$ROOT/Saved/kurulum_derle.log"; }

  for B in 2 3; do
    baslik "K$((B+2)). Oyun: $B. bolum otomatik oynanip bitiyor (ilk acilista shader derlemesi 10-30 dk)"
    rm -f "$ROOT/Saved/Logs/$PROJE.log"   # eski logdaki satir yaniltmasin
    T0=$(date +%s)
    MSYS_NO_PATHCONV=1 timeout 3600 "$UE" "$(cygpath -w "$UPROJ")" "/Game/$PROJE/Maps/L_Test?Bolum=$B" \
      -game -windowed -ResX=1600 -ResY=900 -ExecCmds="SLTestLevel$B" -unattended -log > /dev/null 2>&1
    T1=$(date +%s)
    S=$(grep -a "SL_TESTL${B}_DONE" "$ROOT/Saved/Logs/$PROJE.log" 2>/dev/null | sed 's/.*SL_TESTL/SL_TESTL/' | tr -d '\r')
    if echo "$S" | grep -q "tamam=1"; then tamam "$S ($(sure $((T1-T0))))"
    elif [ -n "$S" ]; then bozuk "$S: bolum bitmedi"
    else bozuk "test satiri yok ($(sure $((T1-T0)))): oyun acilmadi ya da coktu. Saved/Logs/$PROJE.log"; fi
  done

  baslik "K6. Paket + ZIP + temiz klasorde deneme (Tools/teslim_hazirla.sh)"
  T0=$(date +%s)
  bash "$ROOT/Tools/teslim_hazirla.sh" > "$ROOT/Saved/kurulum_teslim.log" 2>&1
  T1=$(date +%s)
  N=$(grep -a "^BOZUK:" "$ROOT/Saved/kurulum_teslim.log" | sed 's/BOZUK: *\([0-9]*\).*/\1/')
  [ "$N" = "0" ] && tamam "teslim kontrolu BOZUK: 0 ($(sure $((T1-T0)))). Gozle bak: Build/Teslim/deneme_ekrani.png" \
    || { bozuk "teslim kontrolu BOZUK: ${N:-?}. Saved/kurulum_teslim.log:"; grep -a "\[BOZUK\]" "$ROOT/Saved/kurulum_teslim.log"; }

  if [ "${TRAILER:-1}" != 0 ]; then
    baslik "K7. Trailer cekimi (Tools/trailer_yap.sh)"
    T0=$(date +%s)
    bash "$ROOT/Tools/trailer_yap.sh" > "$ROOT/Saved/kurulum_trailer.log" 2>&1
    T1=$(date +%s)
    if grep -q "^tamam:" "$ROOT/Saved/kurulum_trailer.log"; then
      D=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$ROOT/Build/Teslim/${PROJE}_Trailer.mp4" 2>/dev/null)
      tamam "trailer ${D%.*} sn ($(sure $((T1-T0))))"
    else
      bozuk "trailer olmadi: Saved/kurulum_trailer.log"; tail -3 "$ROOT/Saved/kurulum_trailer.log"
    fi
    git restore Content/Movies/Intro.mp4 "Build/Teslim/${PROJE}_Trailer.mp4" 2>/dev/null   # deneme reposu eski haline
  fi
fi

baslik "K8. Repo temiz (hicbir sey degismedi, push yok)"
[ -z "$(git status --porcelain)" ] && tamam "git status temiz" || { bozuk "git status temiz degil:"; git status --short; }

{
  echo "KURULUM RAPORU  $(hostname)  $(date '+%d.%m.%Y %H:%M')"
  printf '%s\n' "${SATIR[@]}"
  echo
  [ "$BOZUK" -eq 0 ] && [ "$OLCULEMEDI" -eq 0 ] && echo "SONUC: KURULUM TAMAM" || echo "SONUC: EKSIK VAR (BOZUK $BOZUK, OLCULEMEDI $OLCULEMEDI)"
} > "$RAPOR"
echo
tail -1 "$RAPOR"
echo "rapor: Saved/kurulum_raporu.txt"
[ "$BOZUK" -eq 0 ]
