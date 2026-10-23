# Sallantı

Sallanan bir adaya şehir kur, aileleri mutlu et, dengeyi koru. İzometrik, karton görünüşlü bir denge ve
yerleşim bulmacası. Her bina adanın ağırlık merkezini kaydırır; ağır binayı kenara koyarsan ada devrilir.
Ailelerin istekleri (hastaneye yakın, AVM'den uzak, akrabaya yakın, durağa yolla bağlı...) birbirini zorlar.

CIVIL GAME JAM 2026 · **FreakLabs**

> **Durum:** bu depo jam'in provasıdır (14–15 Ekim 2026, tema açıklanmadan önce). Kural kitapçığı madde 3.1
> gereği önceden hazırlanmış oyun teslim edilemez; bu proje yalnızca araçları ve teslim düzenini denemek
> içindir. Jam oyunu 16 Ekim 18.00'den sonra ayrıca geliştirilecek.

---

## Çalıştırma (kitapçık EK A.3)

**Platform ve gereksinimler:** Windows 10/11, 64 bit, DirectX 12 destekli ekran kartı. Ek yazılım
gerekmez (gerekirse Unreal Engine önkoşulları paketin içindeki `Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe`).

**Başlatma:** ZIP'i bir klasöre açın, `Sallanti.exe`'yi çalıştırın. Açılış videosu bir tıklama ya da tuşla
geçilir; ana menüden **Oyna** ya da **Bölümler**.

**Oynanış:** Amaç her bölümde ailelerin hepsini mutlu etmek (2. ve 3. bölümde mahalle planındaki binalar da
kurulu olmalı). 1. bölüm eğitimdir ve adım adım yönlendirir.

| Girdi | Ne yapar |
|---|---|
| Sol alttaki pusuladan bina seç (ya da 1–6) | Binayı eline al |
| Sol tık | Koy · boştayken bir binayı kaldırıp taşı |
| R / Q | Elindekini 90° çevir (kapının yönü) |
| Aileye tıkla | İsteklerini öğren, seç; seçiliyken ev kur ya da boş eve tıkla |
| Sağ tık sürükle · tekerlek | Haritayı kaydır · yakınlaş |
| Z / X | Tahtayı döndür |
| Esc · sağ tık | Elindekini bırak · menüde geri |
| Backspace | Son konanı geri al |
| F5 / Space | Bölümü baştan başlat (Space yalnızca bölüm bitince) |
| Enter | Sonraki bölüm |
| M | Müzik aç / kapa |

**Bilinen sorunlar:** Gerçek modeller ve yazı tipleri henüz gelmedi; binalar ve figürler kodla çizilen yer
tutucular. Fare üstü efektleri ve fareyle aile seçimi yalnızca elle denenebilir (otomatik testler tıklamanın
işini doğrudan çağırıyor).

## Kaynaktan derleme

Unreal Engine **5.8.0**. Editör kapalıyken `Build\derle.bat`, paket için `Build\paketle.bat`
(Shipping, çıktı `Build\Package\Windows`). Ayrıntılar ve proje kuralları `CLAUDE.md`'de.

Trailer oyundan çekilir: `bash Tools/trailer_yap.sh` (ffmpeg gerekir) →
`Build/Teslim/Sallanti_Trailer.mp4` (≤ 60 sn). Açılış videosu yok; oyun doğrudan ana menüyle açılır.

## Hazır kaynak beyanı (kitapçık 3.4, EK A.4)

| Kategori | Durum | Kaynak · sağlayıcı · lisans · kullanım |
|---|---|---|
| Görsel | Kullanıldı | **Unreal Engine temel şekilleri** (küp, küre, silindir, koni; `/Engine/BasicShapes`) · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · binalar, ağaçlar, figürler, arka plan bunlardan kodla birleştiriliyor; ölçek ve renk değiştirildi. |
| Ses | Kullanılmadı | Bütün efektler ve müzik projede sentezlendi (`Tools/sesleri_uret.py`, `Tools/muzik_uret.py`). |
| Kod | Kullanıldı | **Unreal Engine 5.8** · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · oyun motoru. Python betiklerinde **NumPy** ve **SciPy** (BSD-3) yalnızca ses üretimi için, oyuna girmez. |
| Metin | Kullanılmadı | — |
| Diğer | Kullanıldı | **Cinzel** yazı tipi · The Cinzel Project Authors (Natanael Gama) · [Google Fonts](https://fonts.google.com/specimen/Cinzel) · [SIL OFL 1.1](Art/Fonts/Cinzel/OFL.txt) · başlıklar; değiştirilmedi. **Jost** yazı tipi · The Jost Project Authors (indestructible type*) · [Google Fonts](https://fonts.google.com/specimen/Jost) · [SIL OFL 1.1](Art/Fonts/Jost/OFL.txt) · arayüz metinleri; değiştirilmedi. **Roboto** · Google · [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0) · motorla gelir, font yüklenemezse yedek. |

Repoda yeniden dağıtımı kısıtlı ham asset yoktur (motor içerikleri motorla gelir, repoya konmadı). Cinzel ve
Jost OFL lisanslıdır, yeniden dağıtılabilir; lisans metinleri `Art/Fonts/*/OFL.txt`, oyunun kullandığı
kopyalar `Content/Fonts/`.

## Yapay zekâ beyanı (kitapçık 4.2, EK A.5)

| Kategori | Durum | Araç · amaç · oyundaki yeri |
|---|---|---|
| Görsel | Kullanıldı | **Claude (Anthropic)** · binaların, figürlerin, arka planın ve arayüzün koddan çizimi; arayüz yönü taslakları · bütün görseller (henüz el yapımı model yok). |
| Ses | Kullanıldı | **Claude** · efektleri ve müziği sentezleyen Python betiklerini yazdı · bütün efektler ve arka plan müziği. |
| Kod | Kullanıldı | **Claude** · oyun kodu (C++), test otomasyonu, derleme / video / ses betikleri · bütün kod. |
| Metin | Kullanıldı | **Claude** · aile diyalogları, görev ve ipucu metinleri, menü ve jenerik · oyun içi bütün metinler. |
| Diğer | Kullanıldı | **Claude** · tasarım belgesi (GDD), seviye tasarımı ve dengeleme, trailer çekim senaryosu. |

## Hak sahipliği

Oyun ve özgün içerik üzerindeki haklar ekip üyelerindedir. Kitapçık madde 9 gereği organizasyona ticari
olmayan, münhasır olmayan kullanım izni verilir.
