#!/usr/bin/env bash
# Her commit ve pull'dan sonra oynanabilir build (23 Eki, Yigit). Git hook'u (Tools/otomatik_build_kur.sh
# kurar) bunu arka planda, gizli pencerede baslatir; commit beklemez.
#
#   bash Tools/otomatik_build.sh          # elle: HEAD paketli degilse paketle
#   ZORLA=1 bash Tools/otomatik_build.sh  # HEAD paketli olsa da yeniden paketle
#
# 1) Ayni anda tek kosucu (Saved/otobuild.lock). Paketlerken yeni commit gelirse bittiginde HEAD'e tekrar
#    bakar: ust uste commit'te yalnizca sonuncusu paketlenir.
# 2) Editor aciksa kapanmasini bekler (CLAUDE.md: paketlerken editor kapali).
# 3) Build\paketle.bat -> Build\Package_yeni. Paket logu 10 dk yazilmazsa asilmistir (XGE tuzagi): durdurur.
# 4) Basariliysa Build\Oyna'ya koyar (oyun aciksa kapanmasini bekler); masaustu kisayolu oraya bakar.
#    Build\Oyna\BUILD_BILGISI.txt: commit, tarih, mesaj, kaydedilmemis degisiklik var mi.
# 5) Windows bildirimi: hazir ya da basarisiz. Gunluk: Saved/otobuild.log, paket logu Saved/otobuild_paket.log.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$ROOT/Saved/otobuild.log"
LOCK="$ROOT/Saved/otobuild.lock"
PAKET_LOG="$ROOT/Saved/otobuild_paket.log"
YENI="$ROOT/Build/Package_yeni"
OYNA="$ROOT/Build/Oyna"
ASILMA_SN=600
mkdir -p "$ROOT/Saved"
cd "$ROOT"

log() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOG"; }
calisiyor() { tasklist //FI "IMAGENAME eq $1" 2>/dev/null | grep -qi "$1"; }
bildir() { powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$ROOT/Tools/bildir.ps1")" -Baslik "$1" -Metin "$2" >/dev/null 2>&1 || true; }

if [ -f "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  log "zaten calisiyor (pid $(cat "$LOCK")); $(git rev-parse --short HEAD) bittiginde paketlenecek"
  exit 0
fi
echo $$ > "$LOCK"
trap 'rm -f "$LOCK"' EXIT

SON=""
[ -f "$OYNA/BUILD_BILGISI.txt" ] && SON=$(sed -n 's/^commit: //p' "$OYNA/BUILD_BILGISI.txt" | tr -d '\r')
DENENEN=""
while true; do
  HEAD=$(git rev-parse --short HEAD)
  if [ "${ZORLA:-0}" != 1 ] && { [ "$HEAD" = "$SON" ] || [ "$HEAD" = "$DENENEN" ]; }; then
    log "$HEAD icin yapilacak is yok"
    break
  fi
  ZORLA=0

  BEKLEDI=0
  while calisiyor UnrealEditor.exe; do
    [ $BEKLEDI = 0 ] && log "editor acik: kapanmasini bekliyorum"
    BEKLEDI=1
    sleep 30
  done
  HEAD=$(git rev-parse --short HEAD) # beklerken yeni commit gelmis olabilir
  MESAJ=$(git log -1 --format=%s)
  KIRLI=$([ -n "$(git status --porcelain)" ] && echo "var (paket calisma kopyasindan alindi)" || echo "yok")
  log "$HEAD paketleniyor: $MESAJ"
  rm -rf "$YENI"
  T0=$(date +%s)
  cmd //c "Build\\paketle.bat" "$(cygpath -w "$YENI")" > "$PAKET_LOG" 2>&1 &
  PID=$!
  ASILDI=0
  while kill -0 $PID 2>/dev/null; do
    sleep 20
    YAS=$(( $(date +%s) - $(stat -c %Y "$PAKET_LOG" 2>/dev/null || date +%s) ))
    if [ $YAS -gt $ASILMA_SN ]; then
      ASILDI=1
      log "paket logu $((YAS / 60)) dk yazilmadi: asildi, durduruluyor"
      taskkill //F //T //PID "$(cat /proc/$PID/winpid 2>/dev/null)" > /dev/null 2>&1
      break
    fi
  done
  wait $PID 2>/dev/null
  SURE=$(( $(date +%s) - T0 ))
  DENENEN=$HEAD

  if [ $ASILDI = 1 ] || ! grep -q "BUILD SUCCESSFUL" "$PAKET_LOG" || [ ! -f "$YENI/Windows/Sallanti.exe" ]; then
    HATA=$(grep -a -m1 -E "error |Error:|ERROR:" "$PAKET_LOG" | tr -d '\r' | cut -c1-160)
    log "BASARISIZ $HEAD ($((SURE / 60)) dk $((SURE % 60)) sn): ${HATA:-ayrinti Saved/otobuild_paket.log}"
    bildir "Sallanti build basarisiz" "$HEAD: ${HATA:-Saved/otobuild_paket.log}"
    continue
  fi

  {
    echo "commit: $HEAD"
    echo "tarih: $(date '+%Y-%m-%d %H:%M')"
    echo "mesaj: $MESAJ"
    echo "kaydedilmemis degisiklik: $KIRLI"
    echo "paketleme: $((SURE / 60)) dk $((SURE % 60)) sn"
  } > "$YENI/Windows/BUILD_BILGISI.txt"

  BEKLEDI=0
  while calisiyor Sallanti-Win64-Shipping.exe; do
    [ $BEKLEDI = 0 ] && log "oyun acik: yeni build'i koymak icin kapanmasini bekliyorum"
    BEKLEDI=1
    sleep 15
  done
  rm -rf "$OYNA.eski"
  [ -d "$OYNA" ] && mv "$OYNA" "$OYNA.eski"
  mv "$YENI/Windows" "$OYNA"
  rm -rf "$OYNA.eski" "$YENI"
  SON=$HEAD
  log "HAZIR $HEAD ($((SURE / 60)) dk $((SURE % 60)) sn): Build/Oyna/Sallanti.exe"
  bildir "Sallanti: yeni build hazir" "$HEAD - $MESAJ"
done
