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

**Bitmeyecekler:** envanter, NPC/AI, ayrı state system, yıkım VFX'i, menü. (Karakter animasyonu
listedeydi; 16 Ekim'de Andaç istedi, koddan basit animasyon yapıldı.)
(İkinci seviye bu listedeydi; 15 Ekim'de Andaç 2. bölümü, 16 Ekim'de 3. bölümü istedi, ikisi de yapıldı.
Dördüncü bölüm için tekrar sorulur.)

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
- **Kamera:** −45° (−35'te yol yaparken binaların arkası görünmüyordu), 2900 cm. Tekerlek 1100–3600 cm
  yakınlaştırır. **Sağ tık basılı sürükle = haritayı kaydır** (bakış noktası tahtanın dışına çıkmaz);
  sürüklemeden bırakılan sağ tık eskisi gibi iptal. C = ortala. Hareket bulanıklığı kapalı.
- **Yol görünüşü:** düz/T/kavşak kare karo; iki dik yön = çeyrek daire viraj; tek yön = yuvarlak uç.
  Bina yola bağlanınca kapıdan yola açık renkli giriş yolu (kaldırım). Plaka–gövde payı 8 cm.
- **Ses:** 12 yer tutucu efekt (`Tools/sesleri_uret.py` sentezler → `Art/Audio`, `Tools/sesleri_import.py`
  → `/Game/Sallanti/Audio`). Gerçek ses aynı adla `Art/Audio`'ya konup import betiği tekrar çalıştırılır
  (betik eskisini siler, üstüne import etmez). `ASLGameMode::PlaySfx`: kare başına tek ses, en önemlisi;
  önemli sesten sonra 0,4 sn önemsizler susar ("bir olay, bir ses"). Gıcırtı döngüsü eğimle yükselir.
- **Cozy görünüş (14 Ekim, Andaç'ın referans görseli):** lavanta arka plan, aşağıda bulutlar, kenarlarda
  havada pastel adacıklar (`ASLAmbience`, seviyede yoksa GameMode doğurur). Nane karo + krem derz, tahtanın
  altında kemerli sütunlu pembe kaide (`ASLPlatform::BuildArcade`). Mesh gelene kadar binalar koddan
  süslü (`ASLPlaceable::BuildDecor`): Ev'e mercan beşik çatı + baca, Hastane'ye kırmızı haç, AVM'ye vitrin,
  tente, "AVM" tabelası; hepsine pencere, plakada servi ve çalı. `DisplayMesh` gelince süs çizilmez.
  HUD krem yuvarlak kartlar (turkuaz / sarı / mercan), çekmece "Binalar" + kart başına "Yerleştir".
  **Pozlama sabit** (`ASLCameraPawn::FixedExposureEV100` 1,9): otomatik pozlama büyük arka plana bakıp
  sahneyi karartıyordu. Güneş yönü ve şiddeti `ASLAmbience`'ta (oyun sırasında; seviye dosyası değişmez).
- **Şehir planı:** istif kapalı (`bAllowStacking`), dolu hücre geçersiz.
- **Tahta (14 Ekim, Blender taslağı):** 13×13 hücre, 50 cm → 650 cm. Karo + derz (renkleri cozy maddesinde)
  (`TileGap` 8 cm). Bina **plakada** (ayak izi − derz), gövde plakadan `BodyInsetCm` içeride.
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
- **2. bölüm: Otobüs durağı** (15 Ekim, Andaç; `ASLLevelDirector::ConfigureLevel(2)`). İki aile, mutluluk:
  1–2. Kaya ailesine ev, yerleşir → 3–4. Hastane, AVM → 5. yollarla bağla (Kaya mutlu: evinin yolu
  hastaneye ve AVM'ye değiyor) → 6. **Durak**: önü (sarı kare) Ev+Hastane+AVM'yi birleştiren yola
  bakmalı, yoksa konmaz → otobüs (`ASLBus`) o ağda duraktan en uzak karoya gidip gelir, durakta bekler →
  7–8. Demir ailesi duraktan iner, ona ev kur → 9. **iki aile de mutlu**. Demir'in istekleri (16 Ekim):
  evi durağa en fazla **4 birim** (hücre, |dx|+|dy|, `FSLNeed::MaxCells`; çapraz komşu = 2, yani durağın
  3×3 çevresi her zaman uygun) **ve** evinin kapısından durağa yol. Uzaksa mutsuz (bar %10–45).
  Ev elindeyken isteği tutan boş hücreler sarı yanar, ipucu her isteği tek tek söyler. Sol altta
  "Aileler" kartı (yüz, bar, karşılanmayan istek), evlerin üstünde yüz. Mutsuz aile son görevde şikâyet eder.
  1. bölüm bitince **Enter** 2. bölümü açar. Bölüm numarası URL'de: `L_Test?Bolum=2`
  (`ASLGameMode::LevelNumber`); F5/Space aynı bölümü baştan açar. Çekmece bölümün hiç açmadığı slotu
  göstermez (1. bölümde Durak, 1–2'de Park).
- **3. bölüm: Komşular** (16 Ekim, Andaç: "Is This Seat Taken?" gibi istekler; `ConfigureLevel3`). Üç
  ailenin istekleri birbirini zorlar: **Yılmaz** (yaşlı) hastaneye ≤3 ve yolla bağlı; **Demir** (çocuklu)
  parka ≤2, AVM'ye ≥4 (`FarFrom`, gürültü); **Aydın** (Yılmazların çocukları, otobüsle) Yılmaz evine ≤2
  (`NearFamily`) ve durağa yolla bağlı. Yeni bina **Park** 2×2 kütle 3 (6. kısayol). Zorluk: platform 8,
  eğim 0,10/cm, devrilme 1,2 sn, **24 hamle** (her hamle 2 saat; biterse "HAMLE BİTTİ"). Test çözümü 15 hamle.
- **Açık bölümler (16 Ekim, Andaç: "görevler sırayla, simülasyon gibi"):** 2. ve 3. bölümde sıralı görev
  yok (`ASLLevelDirector::bOpenLevel`). Tek hedef: herkes mutlu + mahalle planı (`RequiredTypes`) kurulu.
  Binalar baştan açık ama **stoklu** (`SlotLimits`: 3. bölümde 3 Ev, 12 Yol...). Ailelerin istekleri
  **gizli**; ailenin üstündeki sarı "?" işaretine tıklayınca istek kartı açılır, aile seçilir. Seçiliyken
  ev kurmak ya da boş bir eve tıklamak aileyi oraya taşır (`AssignFamilyToHouse`); başka eve tıklayınca
  taşınırlar. Sarı bölge yalnızca istekleri bilinen aile için. Otobüsle gelenler `ArriveWhenType` Durak.
  Aileler kendiliğinden konuşur (bekleme / şikâyet / mutlu cümleleri), bir istek karşılandığı ya da
  bozulduğu an tepki verir (`FSLNeed::MetLine` / `Complaint`), akşam "iyi geceler" der.
- **Aileler (`ASLFamily`):** gövde + baş + saç figürleri, kendileri yürür (evin eski figürleri yalnızca
  1. bölümün komşuları için). Eve varınca kapının önünde dururlar; ev taşınırsa onunla gider. Animasyon:
  nefes, yürürken sekme, mutluyken ara ara zıplama, mutsuzken eğik baş, fare üstündeyken biri zıplar,
  seçilince altta halka. Bina konunca kısa "boing" (`ASLPlaceable::PlayPop`).
- **Tahtayı çevirme:** Z / X kamerayı 90° döndürür (yumuşak); yeni binanın kapısı yine kameraya bakar.
- **Zorluk bölüme göre** (`ASLLevelDirector::bApplyTuning`, BeginPlay'de platforma yazılır): 1. bölüm
  eğitim (12 / 0,08 / 1,5 sn), 2. bölüm 10 / 0,09 / 1,3 sn, 3. bölüm 8 / 0,10 / 1,2 sn + hamle sınırı.
- **Gece-gündüz:** her başarılı koyma/taşıma bir hamle (`USLBuildComponent::GetMoveCount`; geri alma ve
  aynı yere geri koyma saymaz). Saat = 07:00 + hamle × `HoursPerMove` (1. ve 2. bölüm 1 saat, 3. bölüm 2).
  `ASLAmbience` saati yumuşakça ilerletir: güneş döner ve alçalır (en alçak 32°; 14°'de akşam tahta
  kararıyordu), gökyüzü ve bulutlar akşam pembe, gece lacivert, yıldızlar çıkar, pencereler sarı yanar
  (pencereler ışıksız `M_Ghost`). Sağ üstte saat kartı (gün, saat, kalan hamle, müzik düğmesi).
- **Arka plan (16 Ekim):** kenardaki binalar kaldırıldı. Yerine yukarı-aşağı sallanan bahçe adacıkları,
  rüzgârla süzülen sıcak hava balonları, tahtanın çevresinde dönen iki kuş sürüsü, üç katmanda farklı
  hızda kayan bulutlar.
- **Müzik:** `Tools/muzik_uret.py` sentezler (`Art/Audio/MUS_Cozy.wav`, 53 sn dikişsiz döngü, 72 BPM),
  `sesleri_import.py` döngüye alır. `ASLGameMode::MusicVolume` 0,35; **M** ya da saat kartındaki hoparlör
  aç/kapa. Ben dinlemedim, yalnızca ölçtüm (tepe −3 dBFS, RMS −16 dBFS, dikiş farkı örnek adımından küçük).
- **Etkileşimli arayüz:** fare üstündeyken kart kalkar ve turkuaz çerçeve alır, "Yerleştir" koyulaşır,
  kulp dışarı çıkar, imleç el olur; karta basınca kısa küçülme, kilitliye basınca sallanma. Başlıktaki ok
  çekmeceyi kapatır. Bölüm sonunda / oyun bitince tıklanır "Tekrar" ve "N. bölüm" düğmeleri.
- **Yol z-fighting:** üst üste binen yol parçaları (uzantılar, viraj dilimleri, çıkmaz diski) aynı
  yükseklikteydi; parlama dalgası geçerken titriyordu. Şimdi uzantı karodan, komşu uzantılar hücre
  paritesine göre birbirinden `RoadZStepCm` (0,05) alçak. 0,2'de parça kenarları çizgi gibi görünüyordu.
- Katalog (`USLBuildComponent::Slots`): Ev **1×1×1 kütle 2** (3 kişi), Hastane 2×2×3 12, AVM 3×3×4 36,
  Yol 1×1 düz 0,5, **Durak 1×1 kütle 1** (5. kısayol; gövde kutusu gizli, sundurma koddan),
  **Park 2×2 kütle 3** (6. kısayol; kapısız, çimen, göl, ağaç, bank). Mert'in mesh'leri `Slots[i].Mesh`'e bağlanır (mesh varken kod kapı çizmez).

**Açık bölümler ekranda görüldü (16 Ekim, `SLTestLevel2/3`):** "?" işaretli aileler, istek kartı, aile
seçiliyken kurulan eve yürüyüş, kapı önünde duran aileler, sarı bölge, tepkiler, döndürülmüş görünüş, gece.
Testler tıklamanın yaptığı işi (`SelectFamily`, `AssignFamilyToHouse`) doğrudan çağırıyor; **gerçek fareyle
aileye tıklama görülmedi.**

**3. bölüm ekranda görüldü (16 Ekim, `SLTestLevel3`):** sarı bölge (Yılmaz: hastane çevresi; Demir: parka
yakın ve AVM'den uzak), Demir'in evi AVM'ye 3 birim → mutsuz ("Park 3 birim uzakta, AVM çok yakın"),
taşıyınca mutlu, durak + otobüs, Aydın duraktan iner, üçü mutlu → bölüm sonu 15 hamlede, saat 2. gün
13:00. Gece görüntüsü (SLSaat 21) pencereler yanık. Aynı derlemeyle 1. ve 2. bölüm de bitti.
**Görülmedi:** hamle sınırının dolması (HAMLE BİTTİ), fareyle hover/basma animasyonları (test fareyi
koordinatla oynatıyor), müziğin kulağa nasıl geldiği.

**2. bölüm ekranda görüldü (15 Ekim, `SLTestLevel2`):** Kaya mutlu (%100) yollar bağlanınca, durak
yola bakmayınca reddediliyor ("önü yola bakmalı"), doğru yerde otobüs başlıyor (rota 5 karo), Demir
duraktan çıkıp uzak eve (13 birim) yürüyor → %10 mutsuz + şikâyet balonu, evi (9,3)'e (2 birim) taşıyınca
mutlu → bölüm sonu. 1. bölüm aynı derlemeyle yine baştan sona bitti, sonunda "Enter: 2. bölüm".
**Görülmedi:** gerçek Enter tuşuyla bölüm geçişi (test URL ile açtı), 5 tuşu, otobüsün uzun rota / kavşakta davranışı.

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
- Otomatik pozlama cozy sahnede lavantayı (0,55 0,45 0,78) ekranda (43,36,57)'ye indiriyordu → sabit
  pozlama. Ölçüm (EV 1,9): arka plan (213,205,227), karo (150,190,170), kaide (201,186,184).
  Işıksız malzeme (arka plan, bulut) `UnlitBrightness` ile çarpılıyor, yoksa ışıklılardan koyu kalıyor.
- Kamera spec'te 1400 cm'di; 600 cm'lik platform sığmıyordu → 2200; tahta 650'ye çıkınca 2400.

**Oynamak / ölçmek:** `Build\oyna.bat` (önce `derle.bat`). Oyun ekranı (`ASLHUD`): denge barı,
görevler (sağ üst), slot çubuğu (alt), uyarı/bölüm mesajı. H: geliştirici paneli (hücre, geçersizlik
sebebi, CoM, ayarlar).

| Tuş | Ne yapar |
|---|---|
| Çekmece (sağ kenar) / Tab | Aç-kapa, karta tıkla: seç. 1–6 kısayol |
| Sol tık | Seçiliyken koy · boştayken bir binayı kaldır (taşı) |
| Sağ tık sürükle / C | Haritayı kaydır / ortala |
| R / Q | Elindekini saat yönünde / tersine 90° çevir (kapının yönü) |
| Tekerlek | Yakınlaş / uzaklaş |
| Sağ tık (sürüklemeden) / Esc | Bırak · taşırken eski yerine koy |
| Backspace | Son konanı geri al |
| F5 / Space | Baştan (Space yalnızca devrilince ya da bölüm bitince) |
| Enter | Bölüm bitince sonraki bölüm |
| M | Müziği aç / kapa |
| Z / X | Tahtayı sola / sağa çevir |
| Aileye tıkla | İsteklerini öğren, seç (seçiliyken ev kur ya da boş eve tıkla) |
| H | Geliştirici paneli |

Konsol (`~`): `SLKnobs` ayarları listeler, `SLSet <Ad> <Değer>` derlemeden değiştirir. Kalıcı değildir.
`SLTestLevel1` bölümü otomatik oynayıp ölçer, ekran görüntüleri `Saved/Screenshots/WindowsEditor/SL_L1_*.png`;
`SLTestLevel2` 2. bölümü oynar (oyunu `"/Game/Sallanti/Maps/L_Test?Bolum=2"` ile aç; `Build\oyna.bat 2`),
görüntüler `SL_L2_*.png`. `SLTestLevel3` aynı şekilde (`?Bolum=3`, `oyna.bat 3`). `SLBolum N` konsoldan
bölüm değiştirir, `SLSaat 21` saati sabitler (gece denemesi; −1 geri verir). `SLReport` anlık eğim/CoM/görevi loga yazar. Eski `SLSelfTest`/`SLTest8` eski kataloga bağlıydı, kaldırıldı.

Spec çakışması: kesme listesi "R ile yeniden başlat" diyor, girdi tablosu R'yi döndürmeye veriyor.
R döndürme kaldı, yeniden başlatma F5.
Git Bash'ten UE çalıştırırken `/Game/...` yolu için `MSYS_NO_PATHCONV=1` gerekir — ama bu değişken
`cmd //c` çağrısını bozar (bat hiç çalışmaz, eski DLL'le test edilir). İkisini aynı komutta kullanma.
