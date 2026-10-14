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

**Kesme sırası:** 1. HUD denge göstergesi (platformun kırmızı nabzı yeter) 2. ses 3. kazanma sayacı
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

**Tasarım (14 Ekim, ekip kararları):**
- Binalar **sabit**, fiziğe bırakılmaz; platformla birlikte eğilir. Eski "fiziğe bırakma" `5ec0385`'te.
- Eğim `WarnTilt` 8° → platform kırmızı atar. `FailTilt` 10° `FailSeconds` 1,5 sn kesintisiz aşılırsa
  platform binalarla birlikte 45°'ye devrilir, oyun biter. Kararlar sallanma hariç eğime bakar.
- **Taşıma (replace):** boştayken sol tık bir binayı kaldırır. Elde tutulurken ağırlığı dengeden
  çıkar — devrilme sayacı işliyorken kaldırmak kurtarır. Sağ tık eski yerine koyar.
- **Denge barı** üstte ortada (yeşil / sarı 8° / kırmızı 10°, devrilirken geri sayım şeridi). Elindekini
  koymadan önce barda **tahmin işareti** (beyaz / sarı / kırmızı); hayalet de tahmine göre yeşil / sarı / turuncu.
- **Kapı ve yön:** Ev, Hastane, AVM'nin kapısı var (yerel +X yüzü). Yol ancak **kapının önündeki hücreye**
  (yerleştirirken sarı kare) değerse bağlanır. R saat yönü, Q tersi, 4 yön; varsayılan kapı kameraya (−Y).
- **Bina çekmecesi:** sağda karton çekmece, kulptan çekilir ya da tıklanır (Tab). Kartlar: çizim, kütle,
  ölçü, kilit. Kart seçilince çekmece kapanır. 1–4 kısayol olarak duruyor.
- **Aile ve konuşma balonu:** bölüm başında ön kenarda bekler (`FamilyWaitLocal`), görevi balonla verir
  (`FSLTask::Speech`), ev kurulunca o noktadan kapıya yürür (kapı arkadaysa yan sokaktan dolaşır), sonra
  evin üstünden konuşur; görev bitince teşekkür (`DoneSpeech`).
- **Eğitim ipuçları** (barın altında): devriliyor > hayalet (konmaz / devirir / sınıra gelir / kapı dışarı /
  kapı önü dolu / elde) > platform hangi yana yatıyor > görevin ne istediği. `ASLHUD::BuildHint`.
- **Kamera:** tekerlekle 1100–3400 cm arası yakınlaş/uzaklaş (çarpımsal adım, yumuşak).
- **Şehir planı:** istif kapalı (`bAllowStacking`), dolu hücre geçersiz.
- **Tahta (14 Ekim, Blender taslağı):** 13×13 hücre, 50 cm → 650 cm. Yeşil karolar + kahverengi derz
  (`TileGap` 8 cm). Bina gri **plakada** (ayak izi − derz), gövde plakadan `BodyInsetCm` içeride.
  **Yol** plakasız karo; komşu yollara ve kapısı kendine bakan binaya derzi kapatan uzantı çıkarır,
  birleşik şerit olur (`ASLPlatform::RefreshConnections`). Plakayı ve yolu kod çiziyor, modelciden
  yalnızca bina gövdesi gelir.
- **Bağlantı efekti:** bir binanın kapısına yol değdiği an bina halka + parlama, yol karoları kapıdan
  uzaklaştıkça sırayla parlar (`ASLLevelDirector::UpdateConnectionFx`, `ASLPlaceable::PlayConnectFx`).
  Malzeme `M_Ghost`; yeni asset yok.
- **1. bölüm = eğitim / vertical slice** (`ASLLevelDirector`, görevler veri):
  1. Ev kur (yalnızca Ev açık) → 2. aile kameraya bakan kenardan yürüyüp eve girer →
  3. Hastane (açılır) → 4. AVM (açılır) → 5. Yol açılır; Ev, Hastane, AVM aynı yol ağına değsin.
  Bitince "BÖLÜM TAMAMLANDI", Space tekrar.
- Katalog (`USLBuildComponent::Slots`): Ev **1×1×1 kütle 2** (3 kişi), Hastane 2×2×3 12, AVM 3×3×4 36,
  Yol 1×1 düz 0,5. Mert'in mesh'leri `Slots[i].Mesh`'e bağlanır (mesh varken kod kapı çizmez).

**Çalışıyor ve ekranda görüldü:** boş proje paketi (Shipping) açılıyor · temiz klon derleniyor ·
**1. bölüm baştan sona** (`SLTestLevel1`, 14 Ekim): kilitli slot reddi, ailenin yürüyüp yerleşmesi,
yanlış AVM ile DEVRİLİYOR, gerçek fareyle AVM'yi kaldırıp kurtarma, yeni yerine koyma, yolla bağlama,
bölüm sonu. Sayılar elle hesapla aynı (3,2° / 2,5° / 12° / 2,47°, bağlantı 6. karoda).

**Ayrıca görüldü (14 Ekim akşam, `SLTestLevel1`):** çekmece kulpu ve kart, kapı/giriş hücreleri, ters kapının
bağlanmaması (2/3) ve R R ile düzelince 3/3, aile bekleyip yürümesi, balonlar, zoom (2200 → 1499),
AVM için turuncu hayalet + "DEVRİLİR" + kırmızı tahmin işareti.

**Derlendi, ekranda görülmedi:** Space/F5 ile yeniden başlatma, G, H, Tab, Q, taşımayı sağ tıkla iptal,
çekmeceyi **gerçek fareyle sürükleme** (test HUD'un tıklama işlevini koordinatla çağırıyor: bu makinede
imleç kullanıcı tarafından oynatıldığı için gerçek fare güvenilmez) ← 2'yi geçerse yeni özellik durur

**Bozuk:** —

**Tuzaklar (bu projede ödendi):** `M_Cardboard`/`M_Ghost`'ta "Used with Instanced Static Meshes" bayrağı
yoktu → karolar varsayılan gri malzemeyle çizildi (`Tools/malzeme_ism_bayragi.py` açtı). Otomatik pozlama
karton renklerini yıkıyordu → kamera `ExposureBias` −1,5 (ölçüldü: karo 119,155,72, derz 114,84,51).

**Gözlemler:**
- Varsayılanlar ölçülerek değişti: `TiltPerCm` 0,04 → **0,08**, `PlatformMass` 0 → **12**.
  0,04'te devrilme pratikte imkânsız; PlatformMass 0'da kenara konan tek nesne oyunu bitirir.
- AVM (36) bilerek ağır: yanlış köşeye konunca tek başına devirir — eğitimin "taşı" dersi bu.
- Aile figürü spec'te 160 cm; ev 50 cm'e inince geçici **35 cm** yapıldı (`ResidentHeightCm`,
  `ASLFamily::HeightCm`). Kesin boy Asil'in kararı.
- Figürlere evin MID'i verilince motor aynı MID'i paylaştırıyor: figürü boyamak evi boyuyordu. Ayrı
  `UMaterialInstanceDynamic::Create` ile çözüldü. Aynı tuzak her "parçanın rengini ayrı ver" işinde.
- `CollapseAngle` 75'te platformun altı ekranı kaplıyordu → 45.
- Platform kraft kahvesi değil krem görünüyor (otomatik pozlama). Mert'in lighting işi.
- Kamera spec'te 1400 cm'di; 600 cm'lik platform sığmıyordu → 2200; tahta 650'ye çıkınca 2400.

**Oynamak / ölçmek:** `Build\oyna.bat` (önce `derle.bat`). Oyun ekranı (`ASLHUD`): denge barı,
görevler (sağ üst), slot çubuğu (alt), uyarı/bölüm mesajı. H: geliştirici paneli (hücre, geçersizlik
sebebi, CoM, ayarlar).

| Tuş | Ne yapar |
|---|---|
| Çekmece (sağ kenar) / Tab | Aç-kapa, karta tıkla: seç. 1–4 kısayol |
| Sol tık | Seçiliyken koy · boştayken bir binayı kaldır (taşı) |
| R / Q | Elindekini saat yönünde / tersine 90° çevir (kapının yönü) |
| Tekerlek | Yakınlaş / uzaklaş |
| Sağ tık / Esc | Bırak · taşırken eski yerine koy |
| Backspace | Son konanı geri al |
| F5 / Space | Baştan (Space yalnızca devrilince ya da bölüm bitince) |
| H | Geliştirici paneli |

Konsol (`~`): `SLKnobs` ayarları listeler, `SLSet <Ad> <Değer>` derlemeden değiştirir. Kalıcı değildir.
`SLTestLevel1` bölümü otomatik oynayıp ölçer, ekran görüntüleri `Saved/Screenshots/WindowsEditor/SL_L1_*.png`;
`SLReport` anlık eğim/CoM/görevi loga yazar. Eski `SLSelfTest`/`SLTest8` eski kataloga bağlıydı, kaldırıldı.

Spec çakışması: kesme listesi "R ile yeniden başlat" diyor, girdi tablosu R'yi döndürmeye veriyor.
R döndürme kaldı, yeniden başlatma F5.
Git Bash'ten UE çalıştırırken `/Game/...` yolu için `MSYS_NO_PATHCONV=1` gerekir — ama bu değişken
`cmd //c` çağrısını bozar (bat hiç çalışmaz, eski DLL'le test edilir). İkisini aynı komutta kullanma.
