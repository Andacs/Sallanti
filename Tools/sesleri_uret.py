"""Yer tutucu ses efektleri: sentezle, Art/Audio/*.wav'a yaz.

Ses tasarimcisi gelene kadar oyunun sesi olsun diye. Ayni isimle gercek dosya gelince yalnizca WAV
degisir, kod ayni kalir. Bicim GDD'deki istekle ayni: 48 kHz, 16 bit, mono, tepe -1 dBFS.
Calistir:  python Tools/sesleri_uret.py
"""
import os
import wave
import numpy as np

SR = 48000
OUT = os.path.join(os.path.dirname(__file__), "..", "Art", "Audio")
rng = np.random.default_rng(14102026)  # tekrar uretilebilir


def t(sec):
    return np.arange(int(SR * sec)) / SR


def env(n, attack=0.005, decay=0.2):
    """Hizli atak, ussel sonme."""
    x = np.arange(n) / SR
    a = np.clip(x / max(attack, 1e-4), 0, 1)
    return a * np.exp(-x / max(decay, 1e-4))


def lowpass(x, cutoff):
    """Tek kutuplu alcak geciren (karton tokluğu icin yeterli)."""
    a = np.exp(-2 * np.pi * cutoff / SR)
    y = np.empty_like(x)
    acc = 0.0
    for i, v in enumerate(x):
        acc = (1 - a) * v + a * acc
        y[i] = acc
    return y


def tone(freq, sec, decay=0.3, harm=(1.0, 0.35, 0.15)):
    tt = t(sec)
    s = sum(h * np.sin(2 * np.pi * freq * (k + 1) * tt) for k, h in enumerate(harm))
    return s * env(len(tt), 0.004, decay)


def write(name, x, peak_db=-1.0):
    x = np.asarray(x, dtype=np.float64)
    m = np.max(np.abs(x)) or 1.0
    x = x / m * (10 ** (peak_db / 20))
    # Bas/son tiklamasin diye 3 ms yumusatma.
    f = int(0.003 * SR)
    x[:f] *= np.linspace(0, 1, f)
    x[-f:] *= np.linspace(1, 0, f)
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name + ".wav")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((x * 32767).astype("<i2").tobytes())
    print(f"{name}.wav  {len(x) / SR:.2f} sn")


def seq(*parts, gap=0.0):
    out = []
    for p in parts:
        out.append(p)
        if gap:
            out.append(np.zeros(int(gap * SR)))
    return np.concatenate(out)


def mix(a, b, offset=0.0):
    o = int(offset * SR)
    n = max(len(a), o + len(b))
    y = np.zeros(n)
    y[:len(a)] += a
    y[o:o + len(b)] += b
    return y


# --- Yerlestirme: karton "tok" (alcak govde + kisa suzulmus gurultu) ---
n = int(0.28 * SR)
body = np.sin(2 * np.pi * 110 * t(0.28)) * env(n, 0.002, 0.07)
knock = lowpass(rng.standard_normal(n), 1800) * env(n, 0.001, 0.025)
write("SFX_Yerlestir", body * 0.9 + knock * 1.4)

# --- Yol koyma: daha hafif, kisa "tik" ---
n = int(0.12 * SR)
write("SFX_YolKoy", lowpass(rng.standard_normal(n), 2600) * env(n, 0.001, 0.02) + 0.4 * np.sin(2 * np.pi * 220 * t(0.12)) * env(n, 0.001, 0.03), peak_db=-6)

# --- Kaldirma: yukari kayan "pop" ---
tt = t(0.16)
f = 280 + 260 * (tt / tt[-1])
write("SFX_Kaldir", np.sin(2 * np.pi * np.cumsum(f) / SR) * env(len(tt), 0.003, 0.06), peak_db=-3)

# --- Gecersiz / kilitli: kisa alcak vizilti ---
tt = t(0.18)
buzz = np.sign(np.sin(2 * np.pi * 95 * tt)) * 0.5 + np.sin(2 * np.pi * 190 * tt) * 0.3
write("SFX_Gecersiz", lowpass(buzz, 900) * env(len(tt), 0.003, 0.08), peak_db=-8)

# --- Yol baglandi: iki notalik can (E5 -> B5), parlak ---
write("SFX_Baglanti", mix(tone(659.25, 0.7, 0.35), tone(987.77, 0.8, 0.45), 0.09), peak_db=-2)

# --- Gorev tamam: C5 E5 G5 arpej ---
write("SFX_Gorev", mix(mix(tone(523.25, 0.6, 0.3), tone(659.25, 0.6, 0.3), 0.11), tone(783.99, 0.9, 0.5), 0.22), peak_db=-2)

# --- Bolum tamam: C5 E5 G5 C6 arpej + akor ---
arp = tone(523.25, 0.5, 0.3)
for i, fr in enumerate((659.25, 783.99, 1046.5)):
    arp = mix(arp, tone(fr, 0.5, 0.3), 0.13 * (i + 1))
chord = sum(tone(fr, 1.4, 0.8) for fr in (523.25, 659.25, 783.99, 1046.5)) * 0.5
write("SFX_Bolum", mix(arp, chord, 0.55), peak_db=-1)

# --- Gicirti dongusu: egimle sesi acilir. Dikissiz: periyodik zarf, tam periyot uzunlugu ---
L = 2.0
tt = t(L)
creak_env = (np.sin(np.pi * tt / L * 2) ** 2)  # 2 sn'de iki gicirti, bas ve son sifir
base = 160 + 40 * np.sin(2 * np.pi * 0.5 * tt)
saw = 2 * ((np.cumsum(base) / SR) % 1.0) - 1
grit = lowpass(rng.standard_normal(len(tt)), 1200) * 0.5
creak = lowpass(saw * 0.6 + grit, 1500) * creak_env
x = creak / (np.max(np.abs(creak)) or 1)
with wave.open(os.path.join(OUT, "SFX_Gicirti_Dongu.wav"), "wb") as w:  # dongu: bas/son yumusatma yok
    os.makedirs(OUT, exist_ok=True)
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes((x * 10 ** (-3 / 20) * 32767).astype("<i2").tobytes())
print("SFX_Gicirti_Dongu.wav  2.00 sn (dongu)")

# --- Devrilme: asagi kayan gurultu + alcak gumburtu ---
tt = t(1.6)
rumble = lowpass(rng.standard_normal(len(tt)), 300) * env(len(tt), 0.05, 0.7)
fall_f = 220 * np.exp(-tt * 1.6) + 40
fall = np.sin(2 * np.pi * np.cumsum(fall_f) / SR) * env(len(tt), 0.01, 0.9)
thud = np.zeros(len(tt)); k = int(1.1 * SR)
thud[k:] = np.sin(2 * np.pi * 60 * t(1.6 - 1.1)) * env(len(tt) - k, 0.002, 0.15)
write("SFX_Devrilme", rumble * 1.2 + fall * 0.8 + thud * 1.5, peak_db=-1)

# --- Konusma: uc kisa "miriltı" hecesi, rastgele perde ---
sy = []
for fr in (420, 520, 470):
    tt = t(0.07)
    v = np.sin(2 * np.pi * fr * tt) + 0.4 * np.sin(2 * np.pi * fr * 2 * tt)
    sy.append(v * np.sin(np.pi * tt / tt[-1]))
write("SFX_Konusma", seq(*sy, gap=0.025), peak_db=-8)

# --- Cekmece: kisa kayma gurultusu ---
n = int(0.3 * SR)
slide = lowpass(rng.standard_normal(n), 700) * np.sin(np.pi * np.arange(n) / n)
write("SFX_Cekmece", slide, peak_db=-9)

# --- UI tik ---
n = int(0.04 * SR)
write("SFX_UI_Tik", lowpass(rng.standard_normal(n), 4000) * env(n, 0.0005, 0.008) + 0.5 * np.sin(2 * np.pi * 1500 * t(0.04)) * env(n, 0.0005, 0.01), peak_db=-10)
