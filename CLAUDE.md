# Sallantı

Sallanan bir platformun üstüne şehir kuruyorsun; her koyduğun şey dengeyi bozuyor. Yapı devrilmeden
hedef sayıda nesne yerleştirince kazanırsın.

48 saatlik game jam projesi. Unreal Engine 5.8.0 (CL 55116800), Windows PC. 4 kişilik ekip, tek
programcı (Andaç).

GDD ve ortak checklist: https://claude.ai/artifact/9WCAxT3yjJ2QjCgpXKsWGv (yerel kopya `Docs/GDD.html`).
Bağlayıcı spec'ler: `C:\Dev\JamKit\09_MEKANIK_SPEC.md` (kod), `C:\Dev\JamKit\08_ASSET_SPEC.md` (model).

---

## Sert kurallar (onay almadan ihlal edilmeyecek)

1. **Projeyi/modülü yeniden adlandırma.** Proje adı `Sallanti`; `.uproject`, modül,
   `Target.cs` dosyaları, `SALLANTI_API` makrosu ve `DefaultEngine.ini` redirect'leri birbirine
   bağlı. Oyunun ekranda görünen adı ayrı bir şey, o değişebilir.
2. **Seviye dosyasının tek sahibi var: Yiğit.** Başkası `L_Ana` seviyesini kaydetmez. Lighting için
   Mert'e açık bir "seviye sende" penceresi verilir; o sırada Yiğit dokunmaz.
3. **Seviyenin kopyası açılmaz.** Deneme `L_Test`'te yapılır (Andaç'ın) ve oradaki hiçbir şey ana
   seviyeye taşınmaz.
4. **`Content/Vendor/` dokunulmaz.** İndirilen paketler yeniden adlandırılmaz, taşınmaz — UE 5.8
   Blueprint içeren paket yeniden adlandırmasında çöküyor.
5. **36. saatten sonra yeni özellik yok.** Sadece hata ve paketleme.

---

## Kapsam

**Çekirdek mekanik:** nesne seç → sürükle → yerleştir → denge bozulur → düzelt.

**Bitecekler:** mekanik · bir seviye · kazanma/kaybetme · yeniden başlama · çalışan paket

**Bitmeyecekler:** envanter, NPC/AI, ayrı state system, karakter animasyonu, yıkım VFX'i, menü,
ikinci seviye.

**Kesme sırası:** 1. HUD denge göstergesi (ses + nesne kayması yeter) 2. ses 3. kazanma sayacı
(sadece kaybetme + yeniden başlatma kalır)

---

## Sahiplik

| Alan | Kim |
|---|---|
| Kod / oynanış | Andaç |
| Seviye dosyası (`L_Ana`) | Yiğit |
| Asset / sanat | Mert, Asil |
| Lighting | Mert (seviye penceresinde) |
| UI / HUD tasarımı | Yiğit |
| Ses | belirlenmedi |
| Paketleme / teslim | Yiğit (komut hazır: `Build\paketle.bat`) |

---

## Klasör düzeni

- `Content/Sallanti/` — bizim yazdığımız her şey (`Maps/`, `Materials/`, mesh'ler)
- `Content/Vendor/` — indirilen paketler, dokunulmaz
- `Source/Sallanti/` — C++. Sınıf öneki `SL` (`ASLPlatform`, `USLBuildComponent`…)
- `Tools/` — headless Python script'leri
- `Build/` — derleme ve paket komutları; paket çıktısı `Build/Package/` (commit edilmez)

Asla commit edilmez: `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/ Build/Package/`

Binary asset'ler Git LFS'ten geçiyor — `.gitattributes`'a bak. Pointer olduğu 14 Ekim'de
`git show HEAD:<yol>` ile doğrulandı.

---

## Derleme

Editör **kapalı** olmalı (Live Coding kilidi). `Build\derle.bat`:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" SallantiEditor Win64 Development -project="<repo>\Sallanti.uproject" -waitmutex -NoXGE -MaxParallelActions=2
```

`-NoXGE -MaxParallelActions=2` bu makinede şart; normal derleme PCH için sanal bellek
oluşturamıyor. Bat dosyası proje yolunu kendi konumundan bulur, repo nereye klonlanırsa çalışır.
Boş projenin ilk derlemesi: **70 sn** (temiz klonda 67 sn). Editörün soğuk açılışı: **97 sn.**

## Paketleme

Editör kapalı. `Build\paketle.bat`:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="<repo>\Sallanti.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="<repo>\Build\Package" -utf8output -unattended -ubtargs="-NoXGE -MaxParallelActions=2"
```

Ölçülen süre (boş proje): **117 sn** sıcak (derleme ve shader önbelleği hazırken). Soğuk ilk
deneme takılmadan önce ~19 dakikaya varmıştı; jam'de ilk paketi **en az 30 dakika** bütçeleyin.
Teslim konfigürasyonu: **Shipping.** Çıktı: `Build\Package\Windows\Sallanti.exe` (boş proje 639 MB).
14 Ekim'de paket açıldı, `L_Ana` ekranda görüldü.

**XGE kapalı tutulmalı.** `Sallanti.uproject`'te `XGEController` eklentisi kapalı,
`DefaultEngine.ini`'de `r.XGEShaderCompile=0`. Açıkken cook işini bitirip kapanışta
"Exiting ShaderCompilingThread" satırında **2,5 saat asılı kaldı**; logdaki tek iz
`Ensure condition failed: GetShadowIndex() == 0` (XGEControllerModule, named pipe). Paket logu
10 dakika yazılmıyorsa asılmıştır, beklemeyin.

---

## Git akışı

Tek dal: `main`. Uzak depo: https://github.com/Andacs/Sallanti (private). Herkes oraya push eder.
Feature branch + PR 48 saatte maliyeti faydasından fazla.

- Çalışan her şey commit edilir, **her commit push edilir**
- Pull/checkout öncesi **editör kapanır** — açık editör `.uasset` kilidi tutuyor ve bir
  `git checkout`'u yarım uygulayabiliyor
- `.umap`/`.uasset` çakışmasında merge yok, seçim var: `git checkout --theirs|--ours <yol>`.
  Seçimi seviye sahibi yapar.
- İlk push (4 LFS nesnesi, 42 KB) 15 sn sürdü; temiz klon 5 sn. Asset geldikçe bu büyür, jam
  ortasında yeniden ölçün.

---

## Çalışma kuralları

- **Ölç, tahmin etme.** Bir değeri iki kez tahminle değiştirdiysen dur ve ölç.
- **Ekranda görmediğin şey yoktur.** Yazıldığı saat içinde PIE'de görülecek; görülmediyse commit
  mesajında "görülmedi" yazar.
- **Ayar sayıları koda gömülmez**, hepsi `EditAnywhere`. Her sayı bir derleme demek.
- **UI metinlerinde emoji yok.** Fontta olmayan tek glif oyunu saniyelerce dondurabiliyor.
- **Görünür bir şeyi sıfıra ölçekleme**, gizle. Sıfır ölçek → ters çevrilemez matris → donma.
- **Asset yolunu ezberden yazma**, Content Browser'dan kopyala.
- Bilinen motor tuzakları: `C:\Dev\JamKit\04_UNREAL_TUZAKLARI.md`
- Ekip kuralları: `Docs/03_EKIP_KURALLARI.md`

---

## Bilinen durum

**Çalışıyor ve ekranda görüldü:** boş proje paketi (Shipping) açılıyor, `L_Ana` ışık/atmosfer
görünüyor · temiz klon derleniyor ve editör açılıyor · **uygulama sırası 1–6** (14 Ekim, `SLSelfTest`
ile ölçüldü ve ekran görüntüsüyle görüldü): izometrik kamera, sallanma, hayalet, ızgara + geçerlilik
rengi, yerleştirme + istif, CoM → eğim. Gerçek fare ışını da ölçüldü.

**Derlendi, ekranda görülmedi:** —  ← 2'yi geçerse yeni özellik durur

**Bozuk:** —

**Gözlemler (6. adım kararı için):**
- Spec formülü CoM = Σ(m·r)/Σm kütleden bağımsız: kenara konan **tek bir kutu** eğimi anında ~9°'ye
  çıkarıyor. `ASLPlatform::PlatformMass` bunu yumuşatır (0 = spec). Oynayarak dene: 0, 10, 30.
- `TiltSpeed` 25 °/sn → eğim 0,4 sn'de oturuyor; "ağır" değil "atik" hissettiriyor.
- Platform kraft kahvesi değil krem görünüyor (otomatik pozlama); hayaletin yeşili bu yüzden soluk.
  Mert'in lighting işi — kodla düzeltilmedi.
- Kamera spec'te 1400 cm'di; ölçüldü, 600 cm'lik platform sığmıyordu → 2200.

**Oynamak / ölçmek:** `Build\oyna.bat`. Konsolda (`~`) `SLSelfTest` 1–6'yı otomatik ölçer, ekran
görüntüleri `Saved/Screenshots/WindowsEditor/SL_*.png`; `SLReport` anlık eğim/CoM'u loga yazar.
Git Bash'ten UE çalıştırırken `/Game/...` yolu için `MSYS_NO_PATHCONV=1` gerekir — ama bu değişken
`cmd //c` çağrısını bozar (bat hiç çalışmaz, eski DLL'le test edilir). İkisini aynı komutta kullanma.
