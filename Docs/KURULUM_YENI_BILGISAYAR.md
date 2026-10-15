# Yeni bilgisayar kurulumu — FreakLabs (CIVIL GAME JAM 2026)

Bu dosya **hiçbir şey kurulu olmayan** bir Windows bilgisayarı, ekibin Unreal Engine 5.8 C++ projesini
derleyip çalıştırabilen, paketleyip teslim edebilen hâle getirir. İki bölüm var:

- **A. İnsan yapar** (15 dk): Claude'un yapamadığı hesap ve uygulama adımları.
- **B. Claude yapar**: araçları kurar, GitHub'ı bağlar, projeyi indirir, `kurulum_kontrol.sh` ile her şeyi
  dener, rapor yazar. Raporun sonunda **`SONUC: KURULUM TAMAM`** yazıyorsa iş bitti.

Deneme projesi: https://github.com/Andacs/Sallanti (public, ekibin provası). Jam oyunu 16 Ekim 18.00'den
sonra ayrı bir repoda başlayacak, aynı araçları kullanacak.

---

## A. İnsan yapar (Claude bunları yapamaz)

1. **Bilgisayar:** Windows 10/11 64 bit, C: sürücüsünde **en az 150 GB boş yer**, yönetici yetkisi, iyi
   internet (Unreal ve Visual Studio onlarca GB indirir). Bilgisayar gece açık kalabilmeli; uyku moduna
   geçmesin (Ayarlar → Sistem → Güç → Ekran ve uyku → prize takılıyken **Hiçbir zaman**).
2. **Git:** https://git-scm.com/download/win → indir, kur, bütün seçenekler varsayılan. (Claude'un komut
   çalıştırabilmesi için Git Bash gerekiyor; bu yüzden ilk iş.)
3. **Claude masaüstü uygulaması:** https://claude.ai/download → kur → **kendi hesabınla** gir → üstte
   **Code** sekmesi. (claude.ai sitesindeki sohbet komut çalıştıramaz; Code sekmesi şart.)
   Klasör olarak `C:\Dev` seç (yoksa önce oluştur).
4. **GitHub hesabı:** yoksa https://github.com/signup ile aç. **Kullanıcı adını Andaç'a gönder**; jam
   reposuna ortak (collaborator) olarak eklenecek. Gelen daveti e-postadan ya da github.com'dan kabul et.
5. **Epic Games hesabı:** yoksa https://www.epicgames.com/id/register ile aç (Unreal'i indirmek için).
6. Code sekmesine şunu yapıştır:

   > https://raw.githubusercontent.com/Andacs/Sallanti/main/Docs/KURULUM_YENI_BILGISAYAR.md
   > Bu dosyayı oku, "B. Claude yapar" bölümünü sırayla uygula. Giriş ve onay gereken yerlerde bana söyle.
   > Sonunda kurulum_kontrol.sh'i çalıştır ve raporu göster.

Kurulum boyunca Claude birkaç kez senden bir şey isteyecek: yönetici onayı ("Evet"), tarayıcıda GitHub
girişi, Epic Launcher'da Unreal'i başlatmak, adın ve e-postan. Testler sırasında fareye dokunma; oyun
penceresi kendi kendine açılıp kapanacak.

---

## B. Claude yapar

### Kurallar

- Her adımın sonucunu **ölç** (sürüm yazdır, dosyaya bak, logda satır ara). "Kurulmuş olmalı" deme.
- **Giriş, şifre, hesap açma ve yönetici onayı insanın işi.** Ne yapacağını tek cümleyle söyle, bitince devam et.
- Uzun kurulumları **paralel** yürüt: Unreal inerken Visual Studio ve küçük araçlar kurulsun.
- Deneme reposuna (`Andacs/Sallanti`) **push etme**, dosyalarını değiştirme. Sonda `git status` temiz olmalı.
- Ekran görüntüsü gerekirse **yalnızca oyun penceresinin** alınır (`Tools/pencere_goruntu.ps1`), masaüstünün değil.
- Bilinen tuzaklar:
  - Git Bash'te `/Game/...` yolu için `MSYS_NO_PATHCONV=1` gerekir, ama bu değişkeni `cmd //c` ile aynı komutta
    kullanma (bat hiç çalışmaz).
  - Unreal'e `.uproject`'i **tam yolla** ver. Göreli yolda motor "Failed to open descriptor file" deyip
    3 saniyede sessizce kapanır (çıkış kodu 1, log yazılmaz).
  - winget ile kurulan araçlar **yeni açılan** terminalde görünür; "bulunamadı" ise terminali yenile.

### B1. Ön kontrol

```powershell
Get-PSDrive C | Select-Object Used, Free
winget --version
git --version
```

- Boş yer 150 GB'tan azsa insana söyle, devam etme.
- `winget` yoksa: Microsoft Store → "Uygulama Yükleyici" (App Installer) güncelle.
- `git` yoksa A.2 yapılmamış: insana söyle.

### B2. Visual Studio (en uzun ikinci iş; hemen başlat)

Proje C++; derleyici olmadan Unreal projeyi **açamaz bile**. Kimliği bul, en yeni **Community** sürümünü seç
(referans bilgisayar: Visual Studio Community 2026, sürüm 18, MSVC 14.50):

```powershell
winget search "Visual Studio Community"
```

```powershell
winget install --id <ID> -e --accept-source-agreements --accept-package-agreements --override "--passive --wait --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Component.Windows11SDK.26100 --includeRecommended"
```

İş yükleri: **C++ ile oyun geliştirme** + **C++ ile masaüstü geliştirme** + **Windows 11 SDK (10.0.26100)**.
Yönetici onayı çıkar, insan onaylar. Bilgisayarda zaten Visual Studio varsa yeniden kurma; Visual Studio
Installer → "Değiştir" ile bu iş yüklerini ekle.

### B3. Unreal Engine 5.8.0 (en uzun iş; insan başlatır)

```powershell
winget install --id EpicGames.EpicGamesLauncher -e --accept-source-agreements --accept-package-agreements
```

Sonra insana söyle:

1. Epic Games Launcher'ı aç, kendi Epic hesabınla gir.
2. Sol menü **Unreal Engine** → üstte **Kitaplık** → "Motor Sürümleri" yanındaki **+** → sürüm **5.8.0**.
3. **Yükle** → konum **tam olarak** `C:\Program Files\Epic Games\UE_5.8` (ekibin betikleri bu yolu kullanıyor).
   Seçenekler varsayılan; "Hata ayıklama için editör sembolleri" **seçilmesin** (çok büyük).
4. İndirme bitince Claude'a haber ver.

İndirme sürerken devam et.

### B4. Küçük araçlar

```powershell
winget install --id GitHub.cli -e --accept-source-agreements --accept-package-agreements
winget install --id Gyan.FFmpeg -e --accept-source-agreements --accept-package-agreements
winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements
```

Yeni terminalde:

```powershell
git lfs install
python --version
python -m pip install numpy scipy pillow
```

`python --version` **3.12** yazmalı (ekibin betikleri `python` komutunu çağırıyor). Microsoft Store açılıyorsa:
Ayarlar → Uygulamalar → Gelişmiş uygulama ayarları → **Uygulama yürütme diğer adları** → `python.exe` ve
`python3.exe` kapat, terminali yenile.

### B5. GitHub'ı bağla

İnsan yapar (tarayıcı açılır, ekranda çıkan kodu girer):

```powershell
gh auth login --hostname github.com --git-protocol https --web
```

Claude yapar. Adı ve e-postayı insana sor. E-postası herkese görünmesin istiyorsa GitHub → Settings →
Emails → "Keep my email addresses private" açıp oradaki `...@users.noreply.github.com` adresini kullan
(repolar public; commit e-postası herkese görünür):

```powershell
git config --global user.name "<Ad Soyad>"
git config --global user.email "<e-posta>"
git config --global core.autocrlf true
gh auth setup-git
gh auth status
```

`gh auth status` "Logged in to github.com account <kullanıcı adı>" demeli.

### B6. Deneme projesini indir

```powershell
New-Item -ItemType Directory -Force C:\Dev | Out-Null
git clone https://github.com/Andacs/Sallanti C:\Dev\Sallanti
```

### B7. Kontrol (Unreal ve Visual Studio bittikten sonra)

**Git Bash**'te (Git ile geldi; Unreal editörü kapalı olmalı):

```bash
cd /c/Dev/Sallanti
bash Tools/kurulum_kontrol.sh
```

Betik sırayla dener; her madde TAMAM / BOZUK / ÖLÇÜLEMEDİ yazar:

| | Ne dener | Süre (referans → yeni bilgisayarda ilk kez) |
|---|---|---|
| K1 | Git, LFS, gh, FFmpeg, Python 3.12 + paketler, Windows tar, Unreal 5.8.0, Visual Studio + C++ araçları, Windows SDK | saniyeler |
| K2 | GitHub girişi, git kimliği, repoya erişim, LFS dosyaları gerçekten inmiş mi | saniyeler |
| K3 | C++ derleme (`Build/derle.bat`) | 2 sn → birkaç dk |
| K4 | Oyun açılıyor, 2. bölüm kendi kendine oynanıp bitiyor | 1 dk 10 sn → **10–30 dk** (ilk shader derlemesi; pencere donuk görünebilir, bekle) |
| K5 | 3. bölüm aynı şekilde | 1 dk 18 sn |
| K6 | Shipping paket + gerçek ZIP + ZIP'i temiz klasörde açıp oyunu başlatma + video + repo kontrolü | 3 dk 46 sn → 20–30 dk |
| K7 | Trailer çekimi (oyun 3. bölümü kendi oynar, FFmpeg video yapar) | 6 dk 44 sn |
| K8 | Repo temiz kaldı mı | saniyeler |

Referans bilgisayarda bütün betik ~14 dk sürdü ve `KURULUM TAMAM` verdi (15 Ekim 22.30). Bitince `Saved/kurulum_raporu.txt` oluşur. Son satır **`SONUC: KURULUM TAMAM`** ise kurulum bitti.
K6'dan sonra `Build/Teslim/deneme_ekrani.png`'ye de bak: oyunun ana menüsü ("SALLANTI" başlığı, solda menü,
sağda yeşil kareli ada) görünmeli. Raporu ve bu görüntüyü sohbette insana göster; insan Andaç'a gönderir.

Bir madde BOZUK ise betik nedenini ve logun yerini yazar; aşağıdaki tabloya bak, düzelt, betiği yeniden
çalıştır. Yalnızca bir kısmı tekrarlamak için: `TRAILER=0 bash Tools/kurulum_kontrol.sh` (trailer'sız).

### B8. Jam reposuna eklendikten sonra (16 Ekim)

Andaç seni ortak ekleyip davet kabul edildiyse, yazma iznini doğrula (`<sahip>/<repo>` Andaç'tan):

```bash
gh api repos/<sahip>/<repo> -q .permissions.push
```

`true` yazmalı. Sonra jam reposunu `C:\Dev\` altına klonla; çalışma kuralları o reponun `CLAUDE.md`'sinde.

---

## Sorun çıkarsa

| Belirti | Ne yap |
|---|---|
| K1 "Visual Studio var ama C++ araçları yok" | Visual Studio Installer → Değiştir → "C++ ile oyun geliştirme" |
| K3 "Unable to find valid toolchain" / SDK bulunamadı | Hata hangi MSVC ya da SDK sürümünü istiyorsa Installer'dan o bileşeni ekle |
| K3 bellek / PCH hatası | `derle.bat` zaten `-MaxParallelActions=2`; diğer programları kapat, tekrar dene |
| K4 "test satırı yok", birkaç saniyede bitti | Proje yolu ya da Unreal yolu yanlış; `Saved/Logs/Sallanti.log` yoksa motor projeyi hiç açmadı |
| K4 "modules are missing" | K3'ü geçir; editörü açık bırakma |
| K6 paket 10 dakika log yazmadan bekliyor | Asılmıştır: süreci kapat, `Build/paket_son.log`'un **başındaki** hatalara bak |
| K2 LFS dosyaları 130–200 bayt | `git lfs install` ve `git lfs pull` |
| `gh` hatası | `gh auth status`; gerekirse `gh auth login` tekrar |
| `python` Store'u açıyor | B4'teki "Uygulama yürütme diğer adları" |

Referans bilgisayardaki sürümler (15 Ekim 2026): Unreal Engine 5.8.0, Visual Studio Community 2026 (18,
MSVC 14.50), Windows SDK 10.0.26100, Git 2.55 + LFS 3.7, GitHub CLI 2.96, FFmpeg 9.0, Python 3.12
(numpy 2.5, scipy 1.18, pillow 12.3).
