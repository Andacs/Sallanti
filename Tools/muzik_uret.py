"""Yer tutucu cozy arka plan muzigi: sentezle, Art/Audio/MUS_Cozy.wav'a yaz.

Ses tasarimcisi gelene kadar. Ayni isimle gercek parca gelince yalnizca WAV degisir, kod ayni kalir
(Tools/sesleri_import.py MUS_Cozy'yi donguye alir).
Bicim: 48 kHz, 16 bit, stereo, tepe -3 dBFS, dikissiz dongu (sonu basina sarili: notalar ve yanki
dongu sinirinda kesilmez).
Parca: 72 BPM, Do majorde Cmaj7 - Am7 - Fmaj7 - G6 (ikiser olcu) x2 = 16 olcu, ~53 sn.
Katmanlar: yumusak pad, elektrik piyano (FM), sinus bas, muzik kutusu melodisi, cok hafif shaker.
Calistir:  python Tools/muzik_uret.py
"""
import os
import wave
import numpy as np
from scipy.signal import butter, sosfilt

SR = 48000
BPM = 72
BEAT = 60.0 / BPM
BAR = 4 * BEAT
BARS = 16
L = int(round(BARS * BAR * SR))
OUT = os.path.join(os.path.dirname(__file__), "..", "Art", "Audio", "MUS_Cozy.wav")
rng = np.random.default_rng(15102026)


def midi(n):
    return 440.0 * 2 ** ((n - 69) / 12)


# Akorlar (MIDI): Cmaj7, Am7, Fmaj7, G6
CHORDS = [
    [48, 52, 55, 59],  # C E G B
    [45, 52, 55, 60],  # A E G C
    [41, 48, 52, 57],  # F C E A
    [43, 50, 52, 59],  # G D E B
]
ROOTS = [36, 33, 29, 31]


def place(buf, start, sig, pan=0.0):
    """Sinyali tampona dongusel ekle (sonu basa sarilir). pan -1 sol .. +1 sag."""
    n = len(sig)
    gl, gr = np.cos((pan + 1) * np.pi / 4), np.sin((pan + 1) * np.pi / 4)
    idx = (start + np.arange(n)) % L
    np.add.at(buf[0], idx, sig * gl)
    np.add.at(buf[1], idx, sig * gr)


def adsr(n, a, d, s, r_start, r):
    x = np.arange(n) / SR
    e = np.where(x < a, x / max(a, 1e-4), s + (1 - s) * np.exp(-(x - a) / max(d, 1e-4)))
    rel = np.clip((x - r_start) / max(r, 1e-4), 0, 1)
    return e * (1 - rel)


def pad(freqs, sec, detune):
    n = int(sec * SR)
    x = np.arange(n) / SR
    s = np.zeros(n)
    for f in freqs:
        for d in (-detune, 0.0, detune):
            ph = rng.uniform(0, 1)
            s += 2 * (((f * (1 + d)) * x + ph) % 1.0) - 1  # testere
    sos = butter(2, 900, btype="low", fs=SR, output="sos")
    s = sosfilt(sos, s) / (len(freqs) * 3)
    return s * adsr(n, 0.9, 1.0, 0.85, sec - 1.0, 1.0)


def epiano(f, sec, vel):
    n = int(sec * SR)
    x = np.arange(n) / SR
    idx = 1.6 * np.exp(-x / 0.35)
    s = np.sin(2 * np.pi * f * x + idx * np.sin(2 * np.pi * f * x))
    s += 0.25 * np.sin(2 * np.pi * 2 * f * x) * np.exp(-x / 0.25)
    return vel * s * np.exp(-x / 1.2) * np.clip(x / 0.004, 0, 1)


def bass(f, sec):
    n = int(sec * SR)
    x = np.arange(n) / SR
    s = np.sin(2 * np.pi * f * x) + 0.18 * np.sin(2 * np.pi * 2 * f * x)
    return s * adsr(n, 0.02, 0.6, 0.6, sec - 0.15, 0.15)


def bell(f, sec):
    n = int(sec * SR)
    x = np.arange(n) / SR
    s = (np.sin(2 * np.pi * f * x) + 0.4 * np.sin(2 * np.pi * 2.0 * f * x) * np.exp(-x / 0.3)
         + 0.15 * np.sin(2 * np.pi * 3.0 * f * x) * np.exp(-x / 0.15))
    return s * np.exp(-x / 0.9) * np.clip(x / 0.003, 0, 1)


def shaker(sec):
    n = int(sec * SR)
    s = rng.standard_normal(n)
    sos = butter(2, [5000, 11000], btype="band", fs=SR, output="sos")
    return sosfilt(sos, s) * np.exp(-np.arange(n) / SR / 0.04)


buf = np.zeros((2, L))
pad_bus = np.zeros((2, L))
s = lambda beats: int(round(beats * BEAT * SR))

# Melodi: C majör pentatonik, olcu basina 2-4 nota; ikinci yarida ust oktav agirlikli cesitleme.
PENTA = [72, 74, 76, 79, 81, 84]
for bar in range(BARS):
    ci = (bar // 2) % 4
    ch = CHORDS[ci]
    t0 = bar * 4
    # Pad: iki olculuk akor, olcu cifti basinda
    if bar % 2 == 0:
        p = pad([midi(n) for n in ch], 2 * BAR + 1.0, 0.004)
        place(pad_bus, s(t0), p * 0.9, -0.2)
        place(pad_bus, s(t0), pad([midi(n + 12) for n in ch[1:]], 2 * BAR + 1.0, 0.006) * 0.35, 0.3)
    # Elektrik piyano: 1. ve 3. vurusta akor, hafif arpej
    for beat in (0, 2.5) if bar % 2 else (0, 2):
        for k, n in enumerate(ch):
            place(buf, s(t0 + beat) + k * int(0.018 * SR), epiano(midi(n + 12), 2.6, 0.16), -0.15 + 0.1 * k)
    # Bas: kok ve besli
    place(buf, s(t0), bass(midi(ROOTS[ci]), 2 * BEAT) * 0.32)
    place(buf, s(t0 + 2), bass(midi(ROOTS[ci] + 7), 2 * BEAT) * 0.26)
    # Melodi
    pattern = [(0.5, 1), (1.5, 0), (3.0, 1)] if bar % 4 in (0, 2) else [(1.0, 0), (2.5, 1)]
    for beat, step in pattern:
        hi = 1 if bar >= 8 else 0
        n = PENTA[(ch[step + 1] % 12 + bar + hi * 2) % len(PENTA)] + (12 if (bar >= 8 and step) else 0)
        place(buf, s(t0 + beat), bell(midi(n), 2.0) * 0.12, 0.35)
    # Shaker: ara vuruslar, cok hafif
    for e in range(8):
        if e % 2 == 1:
            place(buf, s(t0 + e * 0.5), shaker(0.12) * (0.035 if e % 4 == 3 else 0.022), 0.5)

mix = buf + pad_bus

# Dongusel yanki: IR = sonumlu gurultu (sol ve sag ayri), FFT carpimi ile dairesel evrisim.
def reverb_ir(seed, sec=2.2):
    r = np.random.default_rng(seed)
    n = int(sec * SR)
    ir = r.standard_normal(n) * np.exp(-np.arange(n) / SR / 0.55)
    sos = butter(1, 4500, btype="low", fs=SR, output="sos")
    ir = sosfilt(sos, ir)
    full = np.zeros(L)
    full[:n] = ir / np.sqrt(np.sum(ir ** 2))
    return full

wet = np.zeros_like(mix)
for c in range(2):
    wet[c] = np.real(np.fft.ifft(np.fft.fft(mix[c]) * np.fft.fft(reverb_ir(7 + c))))
out = mix + 0.35 * wet

# Sicak ton: hafif alcak geciren (dairesel: sonu basa sarmak icin iki tur filtrele, ikinciyi al).
sos = butter(1, 7000, btype="low", fs=SR, output="sos")
for c in range(2):
    out[c] = sosfilt(sos, np.concatenate([out[c], out[c]]))[L:]

out *= 10 ** (-3 / 20) / np.max(np.abs(out))
pcm = (np.clip(out.T, -1, 1) * 32767).astype("<i2")
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with wave.open(OUT, "wb") as w:
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(SR)
    w.writeframes(pcm.tobytes())
print("MUS_Cozy %.1f sn, %d olcu, %s" % (L / SR, BARS, os.path.abspath(OUT)))
