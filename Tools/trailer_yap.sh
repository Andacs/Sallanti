#!/usr/bin/env bash
# Trailer videosunu oyundan ceker (Git Bash). Editor KAPALI, once Build\derle.bat.
#
#   bash Tools/trailer_yap.sh
#
# 1) Oyunu "-benchmark -fps=30" ile acar: her kare oyunda tam 1/30 sn; makine yavas da olsa video akici.
# 2) SLSinema 3. bolumu trailer temposunda oynar, her kareyi REC_f_#####.png olarak kaydeder (yalnizca
#    oyun penceresi; masaustu kaydedilmez).
# 3) ffmpeg: kareler + MUS_Cozy -> Build/Teslim/Sallanti_Trailer.mp4 (<= 60 sn, kural 5.2 ve A.6)
#    (Acilis videosu 23 Ekim'de kaldirildi: oyun dogrudan ana menuyle acilir.)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe"
SHOTS="$ROOT/Saved/Screenshots/WindowsEditor"
OUT="$ROOT/Build/Teslim"
FPS=30
W=1920
H=1080
mkdir -p "$OUT"
rm -f "$SHOTS"/REC_f_*.png

# MSYS_NO_PATHCONV yalnizca bu komutta (cmd //c ile ayni komutta kullanma: CLAUDE.md).
MSYS_NO_PATHCONV=1 "$UE" "$(cygpath -w "$ROOT/Sallanti.uproject")" "/Game/Sallanti/Maps/L_Test?Bolum=3" \
  -game -windowed -ResX=$W -ResY=$H -benchmark -fps=$FPS -ExecCmds="SLSinema" -unattended -log > "$ROOT/Saved/trailer_run.log" 2>&1 || true
N=$(ls "$SHOTS"/REC_f_*.png 2>/dev/null | wc -l)
echo "kare: $N"
[ "$N" -gt 100 ] || { echo "kare yok, Saved/Logs/Sallanti.log'a bak"; exit 1; }

DUR=$(python -c "print(round($N/$FPS,2))")
MUSIC="$ROOT/Art/Audio/MUS_Cozy.wav"
FADE_OUT=$(python -c "print(max(0,$DUR-2.5))")

# Trailer: tum cekim, muzik basta ve sonda yumusak.
ffmpeg -y -loglevel error -framerate $FPS -i "$SHOTS/REC_f_%05d.png" -stream_loop -1 -i "$MUSIC" \
  -vf "scale=${W}:${H}:flags=lanczos,fade=t=in:st=0:d=0.6,fade=t=out:st=$(python -c "print(max(0,$DUR-0.8))"):d=0.8,format=yuv420p" \
  -af "afade=t=in:st=0:d=1.5,afade=t=out:st=$FADE_OUT:d=2.5" -t $DUR \
  -c:v libx264 -preset slow -crf 18 -c:a aac -b:a 192k -movflags +faststart "$OUT/Sallanti_Trailer.mp4"

ffprobe -v error -show_entries format=duration,size -of default=nw=1 "$OUT/Sallanti_Trailer.mp4"
[ "${KEEP:-0}" = 1 ] || rm -f "$SHOTS"/REC_f_*.png
echo "tamam: $OUT/Sallanti_Trailer.mp4"
