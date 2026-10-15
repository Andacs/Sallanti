#!/usr/bin/env bash
# Son teslim kontrolu (CIVIL GAME JAM kitapcigi v09, madde 3, 5, 6 ve EK A). Git Bash'ten, repo kokunden:
#
#   bash Tools/teslim_hazirla.sh              # paketle + ZIP + temiz klasorde calistir + video + repo kontrolu
#   PAKETLE=0 bash Tools/teslim_hazirla.sh    # mevcut Build/Package'i kullan (paket yeni alindiysa)
#
# Editor KAPALI olmali. Trailer once alinmis olmali (Tools/trailer_yap.sh).
# Her madde uc sonuctan biri: TAMAM / BOZUK / OLCULEMEDI. Sonunda Build/Teslim/TESLIM_OZET.txt yazilir;
# formu doldururken bu dosya ve README acik dursun.
# Sallanti provasinda (15 Ekim) olculdu: tamami 3 dk 22 sn (paket 111 sn, ZIP + temiz klasorde deneme ~1,5 dk).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
UPROJ=$(ls *.uproject | head -1)
PROJE="${UPROJ%.uproject}"
OUT="$ROOT/Build/Teslim"
ZIP="$OUT/${PROJE}_Windows.zip"
VIDEO="$OUT/${PROJE}_Trailer.mp4"
WINTAR=/c/Windows/System32/tar.exe   # Git Bash'in tar'i zip yazamaz; Windows'unki yazar
mkdir -p "$OUT"
BOZUK=0
OLCULEMEDI=0
OZET=()
tamam() { echo "  [TAMAM] $*"; OZET+=("TAMAM       $*"); }
bozuk() { echo "  [BOZUK] $*"; OZET+=("BOZUK       $*"); BOZUK=$((BOZUK+1)); }
olcu()  { echo "  [OLCULEMEDI] $*"; OZET+=("OLCULEMEDI  $*"); OLCULEMEDI=$((OLCULEMEDI+1)); }
baslik() { echo; echo "== $*"; }

baslik "1. Git: her sey commit'li ve push'lu mu (madde 3.7: ZIP ile repo ayni kaynaktan)"
if [ -n "$(git status --porcelain)" ]; then
  bozuk "commit edilmemis degisiklik var:"; git status --short | head -20
else
  tamam "calisma agaci temiz"
fi
git fetch -q origin 2>/dev/null || true
HEAD=$(git rev-parse HEAD)
UZAK=$(git rev-parse '@{u}' 2>/dev/null || echo yok)
if [ "$HEAD" = "$UZAK" ]; then tamam "son commit push'lu: ${HEAD:0:7}"; else bozuk "push edilmemis commit var (yerel ${HEAD:0:7}, uzak ${UZAK:0:7})"; fi

baslik "2. Gizli bilgi taramasi (madde 3.7: parola, anahtar, token public repoya girmez)"
GIZLI=$(git grep -nIiE '(password|passwd|parola|secret|api[_-]?key|access[_-]?key|private[_-]?key|token)[[:space:]]*[=:][[:space:]]*[^[:space:]]{6,}' -- . ':!*.md' ':!Tools/teslim_hazirla.sh' || true)
GIZLI_DOSYA=$(git ls-files | grep -iE '(\.env|\.pem|\.key|\.p12|\.pfx|id_rsa|credentials)$' || true)
if [ -z "$GIZLI$GIZLI_DOSYA" ]; then
  tamam "repoda anahtar/parola izi yok"
else
  bozuk "ilk 3.7 adayi bulundu (UE AndroidFileServer SecurityToken'i burada cikar: satiri sil, bEnablePlugin=False):"
  echo "$GIZLI" | head -10; echo "$GIZLI_DOSYA"
fi

baslik "3. Repo public mi, girissiz aciliyor mu (madde 5.3, 6.5: private = diskalifiye)"
REPO=$(gh repo view --json url -q .url 2>/dev/null || echo "")
GORUNUR=$(gh repo view --json visibility -q .visibility 2>/dev/null || echo "")
if [ -z "$REPO" ]; then
  olcu "gh repo view calismadi (gh auth status?)"
else
  [ "$GORUNUR" = "PUBLIC" ] && tamam "repo PUBLIC: $REPO" || bozuk "repo $GORUNUR: gh repo edit --visibility public --accept-visibility-change-consequences"
  KOD=$(curl -s -o /dev/null -w "%{http_code}" "$REPO")
  [ "$KOD" = "200" ] && tamam "girissiz HTTP 200" || bozuk "girissiz istek HTTP $KOD"
fi

baslik "4. README: calistirma talimati + iki beyan, bes kategori (madde 3.4, 4.2, 5.2, EK A.3-A.5)"
if [ ! -f README.md ]; then
  bozuk "README.md yok (sablon: JamKit/araclar/README_SABLON.md)"
else
  for k in "Platform" "Başlatma" "Oynanış" "Bilinen sorunlar" "Hazır kaynak" "Yapay zekâ"; do
    grep -q "$k" README.md && tamam "README'de '$k' var" || bozuk "README'de '$k' bolumu yok"
  done
  for k in "Görsel" "Ses" "Kod" "Metin" "Diğer"; do
    N=$(grep -c "^| $k " README.md)
    [ "$N" -ge 2 ] && tamam "'$k' iki beyanda da var" || bozuk "'$k' satiri iki beyan tablosunda birden yok (bulunan: $N)"
  done
  BOS=$(grep -nE '\[ *\]|\[\.\.\.\]|\[Oyun adı\]|\[Proje\]|Kullanıldı / Kullanılmadı|TODO|___' README.md || true)
  [ -z "$BOS" ] && tamam "README'de sablon boslugu kalmamis" || { bozuk "README'de doldurulmamis yer var:"; echo "$BOS" | head -8; }
fi

baslik "5. Paket (Shipping)"
if [ "${PAKETLE:-1}" != 0 ]; then
  if tasklist //FI "IMAGENAME eq UnrealEditor.exe" 2>/dev/null | grep -qi UnrealEditor; then
    bozuk "editor acik: kapat, sonra tekrar calistir"; echo; echo "BOZUK: $BOZUK"; exit 1
  fi
  T0=$(date +%s)
  cmd //c "Build\\paketle.bat" > "$ROOT/Build/paket_son.log" 2>&1
  T1=$(date +%s)
  grep -q "BUILD SUCCESSFUL" "$ROOT/Build/paket_son.log" && tamam "paket $((T1-T0)) sn" || { bozuk "paket basarisiz: Build/paket_son.log (ilk Error satirlarina bak, sondakilere degil)"; grep -m5 -i "error" "$ROOT/Build/paket_son.log"; }
fi
EXE="$ROOT/Build/Package/Windows/$PROJE.exe"
if [ ! -f "$EXE" ]; then
  bozuk "$EXE yok"
else
  COMMIT_T=$(git log -1 --format=%ct)
  EXE_T=$(stat -c %Y "$EXE")
  [ "$EXE_T" -ge "$COMMIT_T" ] && tamam "paket son commit'ten yeni" || bozuk "paket son commit'ten ESKI: yeniden paketle (madde 3.7)"
fi

baslik "6. ZIP"
if [ -f "$EXE" ]; then
  rm -f "$ZIP"
  ( cd "$ROOT/Build/Package/Windows" && "$WINTAR" -a -c -f "$(cygpath -w "$ZIP")" * )
  # Git Bash'in GNU tar'i "-a" ile .zip adli bir TAR yazar, uyarmaz; Gezgin acamaz. Ilk iki bayt "PK" olmali.
  [ "$(head -c 2 "$ZIP" 2>/dev/null)" = "PK" ] && tamam "dosya gercekten ZIP (PK imzasi)" || bozuk "$ZIP ZIP degil (tar mi?): Windows tar.exe ile yeniden olustur"
  BOYUT=$(stat -c %s "$ZIP" 2>/dev/null || echo 0)
  if [ "$BOYUT" -gt 0 ] && [ "$BOYUT" -lt 10737418240 ]; then tamam "ZIP $((BOYUT/1048576)) MB (sinir 10 GB)"; else bozuk "ZIP olusmadi ya da 10 GB'i asiyor ($BOYUT bayt)"; fi
fi

baslik "7. Temiz klasorde calistir (madde 3.2, 6.6: calismayan oyun degerlendirilmez)"
if [ -f "$ZIP" ]; then
  DENEME="$(cygpath -u "$TEMP")/teslim_deneme_$$"
  mkdir -p "$DENEME"
  "$WINTAR" -x -f "$(cygpath -w "$ZIP")" -C "$(cygpath -w "$DENEME")"
  if [ ! -f "$DENEME/$PROJE.exe" ]; then
    bozuk "ZIP'in kokunde $PROJE.exe yok"
  else
    "$DENEME/$PROJE.exe" -windowed -ResX=1280 -ResY=720 &
    sleep "${BEKLE:-45}"   # acilis videosu + menuye gecis
    KARE="$OUT/deneme_ekrani.png"
    SONUC=$(powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$ROOT/Tools/pencere_goruntu.ps1")" -Process "$PROJE-Win64-Shipping" -Out "$(cygpath -w "$KARE")")
    if echo "$SONUC" | grep -q "^OK"; then
      PARLAK=$(python -c "from PIL import Image,ImageStat;print(int(sum(ImageStat.Stat(Image.open(r'$(cygpath -w "$KARE")').convert('L')).mean)))" 2>/dev/null || echo -1)
      if [ "$PARLAK" -gt 8 ]; then tamam "oyun 45 sn sonra acik, pencere bos degil (parlaklik $PARLAK). GOZLE BAK: Build/Teslim/deneme_ekrani.png"
      elif [ "$PARLAK" -lt 0 ]; then olcu "goruntu alindi ama olculemedi, gozle bak: Build/Teslim/deneme_ekrani.png"
      else bozuk "pencere simsiyah (parlaklik $PARLAK): Build/Teslim/deneme_ekrani.png"; fi
    else
      bozuk "oyun penceresi yok ($SONUC): acilista coktu olabilir. $DENEME/$PROJE/Saved/Logs'a bak"
    fi
    taskkill //F //IM "$PROJE-Win64-Shipping.exe" > /dev/null 2>&1
    sleep 3   # surec kapanmadan klasor silinmiyor
    rm -rf "$DENEME" || echo "  (deneme klasoru silinemedi: $DENEME)"
  fi
fi

baslik "8. Video (madde 5.2, 5.3, EK A.6: <= 1 dk, MP4/AVI, <= 5 GB, ZIP'ten ayri)"
if [ ! -f "$VIDEO" ]; then
  bozuk "$VIDEO yok: bash Tools/trailer_yap.sh"
else
  SURE=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$VIDEO")
  AKIS=$(ffprobe -v error -show_entries stream=codec_type -of csv=p=0 "$VIDEO" | sort -u | tr '\n' ' ')
  python -c "import sys;sys.exit(0 if float('$SURE')<=60 else 1)" && tamam "video ${SURE%.*} sn (sinir 60)" || bozuk "video ${SURE} sn: 60'i asiyor"
  echo "$AKIS" | grep -q audio && tamam "videoda ses var" || bozuk "videoda ses yok"
  [ "$VIDEO" -nt "$EXE" ] 2>/dev/null || echo "  (not: video paketten eski; oyun degistiyse trailer'i yeniden al)"
fi

{
  echo "TESLIM OZETI  $(date '+%d.%m.%Y %H:%M')"
  echo "Commit   : $HEAD"
  echo "Repo     : ${REPO:-?}"
  [ -f "$ZIP" ] && echo "ZIP      : $(basename "$ZIP")  $(( $(stat -c %s "$ZIP") / 1048576 )) MB  sha256 $(sha256sum "$ZIP" | cut -c1-16)"
  [ -f "$VIDEO" ] && echo "Video    : $(basename "$VIDEO")  ${SURE:-?} sn"
  echo
  printf '%s\n' "${OZET[@]}"
  echo
  echo "Form: https://game.ktimo.com.tr/jam/teslim  (son: 18 Ekim 18.00, yukleme KAYDI o saatten once)"
  echo "Yukledikten sonra: makbuz no, surum, kayit saati, dosya adlari -> buraya yaz (madde 5.5)"
} > "$OUT/TESLIM_OZET.txt"

echo
echo "BOZUK: $BOZUK   OLCULEMEDI: $OLCULEMEDI   ozet: Build/Teslim/TESLIM_OZET.txt"
[ "$BOZUK" -eq 0 ]
