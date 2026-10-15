# Yeni bilgisayar kurulumu — FreakLabs (CIVIL GAME JAM 2026)

**Bu dosya bir Claude oturumuna verilmek için yazıldı.** Bilgisayarın sahibi (ekip üyesi) bunu kendi
Claude'una verir; Claude adımları sırayla uygular, insan gereken yerlerde bilgisayar sahibine söyler,
sonunda testleri koşar ve rapor yazar. **Bütün testler TAMAM ise kurulum bitmiştir.**

Amaç: bu bilgisayarda ekibin Unreal Engine 5.8 C++ projesini derleyip çalıştırabilmek, paketleyip teslim
kontrolünü yapabilmek. Deneme projesi: https://github.com/Andacs/Sallanti (public, ekibin provası; jam
oyunu 16 Ekim 18.00'den sonra ayrı bir repoda başlayacak, aynı araçları kullanacak).

Referans bilgisayarda çalışan sürümler (15 Ekim 2026): Unreal Engine 5.8.0, Visual Studio Community 2026
(sürüm 18, MSVC 14.50), Windows SDK 10.0.26100, Git 2.55 + Git LFS 3.7, GitHub CLI 2.96, FFmpeg 9.0,
Python 3.12 + numpy, scipy, pillow.

---

## Claude için kurallar

- Komutları sırayla çalıştır, her adımın sonucunu **ölç** (sürüm yazdır, dosyaya bak, logda satır ara).
  "Kurulmuş olmalı" deme.
- **Giriş ve şifre adımlarını insana bırak:** Epic Games, GitHub, Visual Studio yükleyicisinin yönetici
  onayı. Ne yapacağını tek cümleyle söyle, bitince devam et.
- Uzun kurulumları (Unreal, Visual Studio) **paralel** başlat: biri inerken diğerleri kurulsun.
- Deneme reposuna (`Andacs/Sallanti`) **push etme**, dosyalarını değiştirme. Testler bittiğinde
  `git status` temiz olmalı.
- Ekran görüntüsü gerekirse **yalnızca oyun penceresinin** alınır (`Tools/pencere_goruntu.ps1`),
  masaüstünün değil.
- Testler sırasında bilgisayar sahibi fareyi oynatmasın; oyun penceresi açılıp kapanacak.
- Git Bash'te `/Game/...` yolu için `MSYS_NO_PATHCONV=1` gerekir, ama bu değişkeni `cmd //c` ile aynı
  komutta kullanma (bat hiç çalışmaz).
- Unreal'e `.uproject`'i **tam yolla** ver. Göreli yol verilirse motor kendi klasöründe arar, "Failed to
  open descriptor file" deyip 3 saniyede sessizce kapanır (çıkış kodu 1, log yazılmaz).

---

## 0. Ön kontrol

PowerShell:

```powershell
Get-PSDrive C | Select-Object Used, Free
winget --version
```

- C: sürücüsünde **en az 150 GB boş** yer olmalı (Unreal + Visual Studio + proje + shader önbelleği).
  Yoksa bilgisayar sahibine söyle, devam etme.
- `winget` yoksa Microsoft Store'dan "Uygulama Yükleyici" (App Installer) güncellenmeli.

## 1. Hafif araçlar (birkaç dakika)

```powershell
winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
winget install --id GitHub.cli -e
winget install --id Gyan.FFmpeg -e
winget install --id Python.Python.3.12 -e
winget install --id EpicGames.EpicGamesLauncher -e
```

Yeni bir terminal aç (PATH yenilensin), sonra:

```powershell
git lfs install
python --version
python -m pip install numpy scipy pillow
```

`python --version` **3.12** yazmalı (ekibin betikleri `python` komutunu çağırıyor). Microsoft Store açılıyorsa ya
da sürüm yazmıyorsa: Ayarlar → Uygulamalar → Gelişmiş uygulama ayarları → **Uygulama yürütme diğer adları** →
`python.exe` ve `python3.exe` kapat, terminali yeniden aç.

## 2. Visual Studio (uzun; hemen başlat)

Unreal 5.8 projesi C++; derleyici olmadan proje **açılmaz bile**. Önce kimliği bul:

```powershell
winget search "Visual Studio Community"
```

En yeni Community sürümünü seç (referans: 2026 / sürüm 18). Kimliği `<ID>` yerine yazıp şu iş yükleriyle kur:

```powershell
winget install --id <ID> -e --override "--passive --wait --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Component.Windows11SDK.26100 --includeRecommended"
```

- İş yükleri: **C++ ile oyun geliştirme** + **C++ ile masaüstü geliştirme** + **Windows 11 SDK (10.0.26100)**.
- Yönetici onayı penceresi çıkar: insan onaylar.
- Bilgisayarda zaten bir Visual Studio varsa yeniden kurma; Visual Studio Installer'dan bu iş yüklerinin
  ekli olduğunu kontrol et (eksikse "Değiştir" ile ekle).

## 3. Unreal Engine 5.8.0 (en uzun; insan başlatır)

Komut satırından kurulamıyor. Bilgisayar sahibine şunu söyle:

1. Epic Games Launcher'ı aç, **kendi** Epic hesabınla gir.
2. Sol menü **Unreal Engine** → üstte **Kitaplık** → "Motor Sürümleri" yanındaki **+** → sürüm **5.8.0**.
3. **Yükle** → konum **tam olarak** `C:\Program Files\Epic Games\UE_5.8` olmalı (ekibin betikleri bu
   yolu kullanıyor). Seçenekler varsayılan kalsın; "Hata ayıklama için editör sembolleri" **seçilmesin**
   (çok büyük).
4. İndirme bitince Claude'a haber ver.

İndirme sürerken 4. adıma geç.

## 4. GitHub ve deneme reposu

İnsan yapar (tarayıcı açılır):

```powershell
gh auth login
```

Seçenekler: GitHub.com → HTTPS → "Login with a web browser".

Claude yapar. `<Ad Soyad>` ve `<e-posta>` bilgisayar sahibinden sorulur (gizli kalsın isteniyorsa GitHub
ayarlarındaki `...@users.noreply.github.com` adresi):

```powershell
git config --global user.name "<Ad Soyad>"
git config --global user.email "<e-posta>"
git config --global core.autocrlf true
gh auth setup-git
New-Item -ItemType Directory -Force C:\Dev | Out-Null
git clone https://github.com/Andacs/Sallanti C:\Dev\Sallanti
```

Klonda LFS dosyaları gerçek mi, işaretçi mi:

```powershell
cd C:\Dev\Sallanti
git lfs ls-files | Select-Object -First 5
(Get-Item Build\Teslim\Sallanti_Trailer.mp4).Length
```

Trailer 1 MB'tan büyük olmalı (~15 MB). 200 bayt civarındaysa LFS inmemiştir: `git lfs pull`.

Ekibe katılım: jam reposuna push edebilmek için **Andaç'a GitHub kullanıcı adını** bildir; seni ortak
(collaborator) olarak ekleyecek.

---

## 5. Testler (Unreal ve Visual Studio bittikten sonra)

Hepsi **Git Bash**'ten (Git ile geldi), `/c/Dev/Sallanti` içinde. Unreal editörü **kapalı** olmalı.

### T1 — Araç sürümleri

```bash
git --version; git lfs version; gh --version | head -1; ffmpeg -version | head -1
python -c "import numpy, scipy, PIL; print('python paketleri tamam')"
ls "/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe"
```

TAMAM: hepsi sürüm yazar, UnrealEditor.exe bulunur. (`ffmpeg` ya da `python` bulunamazsa terminali yeniden aç.)

### T2 — C++ derleme

```bash
cd /c/Dev/Sallanti
cmd //c "Build\\derle.bat" 2>&1 | tail -5
```

TAMAM: son satırlarda **`Result: Succeeded`**. İlk derleme birkaç dakika sürebilir.
Toolchain/SDK hatası çıkarsa: hata hangi MSVC ya da SDK sürümünü istiyorsa Visual Studio Installer'dan
o bileşeni ekle, tekrar dene.

### T3 — Oyun açılıyor, 2. bölüm otomatik oynanıp bitiyor

İlk açılışta shader'lar derlenir: **10–30 dakika** sürebilir, pencere bu sırada boş/donuk görünebilir.
Bekle; oyun testi bitirince kendisi kapanır.

```bash
cd /c/Dev/Sallanti
MSYS_NO_PATHCONV=1 "/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "$(cygpath -w "$PWD/Sallanti.uproject")" "/Game/Sallanti/Maps/L_Test?Bolum=2" -game -windowed -ResX=1600 -ResY=900 -ExecCmds="SLTestLevel2" -unattended -log
grep -a "SL_TESTL2_DONE" Saved/Logs/Sallanti.log
```

TAMAM: **`SL_TESTL2_DONE tamam=1`**. Referans bilgisayarda (shader önbelleği hazırken) 1 dk 20 sn. `tamam=0` ya da satır yoksa: `grep -a -i "error" Saved/Logs/Sallanti.log | head`.

### T4 — 3. bölüm (en zor bölüm, aynı yöntem)

```bash
MSYS_NO_PATHCONV=1 "/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "$(cygpath -w "$PWD/Sallanti.uproject")" "/Game/Sallanti/Maps/L_Test?Bolum=3" -game -windowed -ResX=1600 -ResY=900 -ExecCmds="SLTestLevel3" -unattended -log
grep -a "SL_TESTL3_DONE" Saved/Logs/Sallanti.log
```

TAMAM: **`SL_TESTL3_DONE tamam=1 hamle=15`** (referansta 1 dk 13 sn).

### T5 — Paket + teslim kontrolü (uçtan uca)

Shipping paketi alır, gerçek ZIP yapar, ZIP'i temiz klasöre açıp oyunu başlatır, oyun penceresinin
görüntüsünü alır, videoyu ve repoyu kontrol eder. Yeni bilgisayarda ilk paket **20–30 dakika** sürebilir.

```bash
cd /c/Dev/Sallanti
bash Tools/teslim_hazirla.sh
```

TAMAM: en sonda **`BOZUK: 0`** (referansta tamamı 3 dk 22 sn). Sonra `Build/Teslim/deneme_ekrani.png`'ye bak: oyunun ana menüsü
("SALLANTI" başlığı, solda menü, sağda yeşil kareli ada) görünmeli.
"video paketten eski" notu normal (bu bilgisayarda trailer çekilmedi), BOZUK sayılmaz.

### T6 — Trailer çekimi (FFmpeg ve oyun içi kayıt)

Birkaç dakika. Oyun penceresi açılıp 3. bölümü kendi kendine oynar, kapanır.

```bash
cd /c/Dev/Sallanti
bash Tools/trailer_yap.sh
git restore Content/Movies/Intro.mp4 Build/Teslim/Sallanti_Trailer.mp4 2>/dev/null; git status --short
```

TAMAM: `tamam: ...Sallanti_Trailer.mp4` yazar, `ffprobe` süresi ~36 sn gösterir. Sondaki `git restore`
deneme reposunu eski hâline döndürür; `git status --short` boş çıkmalı.

---

## 6. Rapor

Bütün testler bitince `C:\Dev\Sallanti\Saved\kurulum_raporu.txt` dosyasına (Saved/ git'e girmez) ve
sohbete şu tabloyu yaz:

```
KURULUM RAPORU — <bilgisayar adı> — <tarih saat>
Unreal      : 5.8.0  (C:\Program Files\Epic Games\UE_5.8)
VS / MSVC   : <sürüm>      SDK: <sürüm>
Git / LFS   : <sürüm>      gh: <sürüm>, giriş: <kullanıcı adı>
FFmpeg      : <sürüm>      Python: <sürüm>
T1 araçlar        : TAMAM / BOZUK / ÖLÇÜLEMEDİ
T2 derleme        : ...   (<süre>)
T3 bölüm 2 testi  : ...   (<süre, ilk shader derlemesi dahil>)
T4 bölüm 3 testi  : ...
T5 paket + teslim : ...   (<süre>, BOZUK: <n>)
T6 trailer        : ...
git status        : temiz / değil
SONUÇ: KURULUM TAMAM  (ya da: eksikler ...)
```

Bilgisayar sahibi bu raporu Andaç'a gönderir. **Altı test TAMAM ve `git status` temizse kurulum bitti.**

## Sorun çıkarsa

| Belirti | Ne yap |
|---|---|
| `derle.bat` "Unable to find valid toolchain" / SDK bulunamadı | Visual Studio Installer → eksik MSVC/SDK bileşenini ekle |
| Derleme bellek/PCH hatası | `derle.bat` zaten `-MaxParallelActions=2` kullanıyor; diğer programları kapat, tekrar dene |
| Paket 10 dakika log yazmadan bekliyor | Asılmıştır (XGE projede kapalı ama yine de): süreci kapat, `Build/paket_son.log`'un **başındaki** hatalara bak |
| Oyun açılmıyor, "modules are missing" | T2'yi tekrar çalıştır; editörü açık bırakma |
| `teslim_hazirla.sh` "repo PUBLIC değil" ya da `gh` hatası | `gh auth status` |
| LFS dosyaları 130–200 bayt | `git lfs install && git lfs pull` |

Daha fazla Unreal tuzağı: Andaç'taki jam kitinde (`04_UNREAL_TUZAKLARI.md`); jam reposuna da konacak.
