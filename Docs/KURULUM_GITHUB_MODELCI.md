# Modelci için GitHub kurulumu — FreakLabs (CIVIL GAME JAM 2026)

**Bu dosya bir yapay zekâ asistanına (Antigravity / Gemini, Claude vb.) verilmek için yazıldı.**
Bilgisayarın sahibi (ekibin modelcisi) bunu asistanına verir; asistan adımları sırayla uygular, giriş
gereken yerlerde insana söyler, sonda kontrol tablosunu doldurur.

Amaç yalnızca **GitHub**: modelcinin bilgisayarı ekibin reposunu indirebilsin ve modelleri (FBX, doku,
.blend) doğru klasöre push edebilsin. Unreal Engine **kurulmaz**; modeller Unreal'e programcı (Andaç)
tarafından aktarılır.

Deneme reposu: https://github.com/Andacs/Sallanti (public, ekibin provası). Jam reposu 16 Ekim 18.00'den
sonra açılacak; adını Andaç bildirecek.

---

## A. İnsan yapar (asistan yapamaz)

1. GitHub hesabı yoksa https://github.com/signup ile aç. **Kullanıcı adını Andaç'a gönder.**
2. Andaç jam reposuna ortak (collaborator) olarak ekleyince gelen daveti kabul et (e-posta ya da
   https://github.com/notifications). Davet 16 Ekim akşamı gelecek.
3. Asistanın terminal komutu çalıştırabildiği bir pencerede (Antigravity'de agent / terminal) bu dosyayı ver:

   > https://raw.githubusercontent.com/Andacs/Sallanti/main/Docs/KURULUM_GITHUB_MODELCI.md
   > Bu dosyayı oku, "B. Asistan yapar" bölümünü sırayla uygula. Giriş gereken yerde bana söyle, sonda kontrol tablosunu göster.

---

## B. Asistan yapar

### Kurallar

- Her adımın sonucunu **kontrol et** (sürüm yazdır, çıktıya bak). Varsayma.
- **Şifre girme, hesap açma, tarayıcı girişi insanın işi.** Ne yapacağını söyle, bitince devam et.
- Deneme reposuna (`Andacs/Sallanti`) **push etme**; yazma izni de yok.
- Ekran görüntüsü alma.

### B1. Git, Git LFS, GitHub CLI

PowerShell:

```powershell
git --version
gh --version
```

Eksik olanı kur, sonra **yeni bir terminal** aç:

```powershell
winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
winget install --id GitHub.cli -e --accept-source-agreements --accept-package-agreements
```

```powershell
git lfs install
git lfs version
```

`git lfs version` sürüm yazmalı (Git for Windows ile gelir). Modeller ve dokular büyük dosya; LFS olmadan
repoya düzgün girmez.

### B2. GitHub'a giriş

İnsan yapar (tarayıcı açılır, ekrandaki kodu girer):

```powershell
gh auth login --hostname github.com --git-protocol https --web
```

Asistan yapar. Adı ve e-postayı insana sor. Repolar **public**, commit e-postası herkese görünür; gizli
kalsın isteniyorsa GitHub → Settings → Emails → "Keep my email addresses private" açılır ve oradaki
`...@users.noreply.github.com` adresi kullanılır:

```powershell
git config --global user.name "<Ad Soyad>"
git config --global user.email "<e-posta>"
git config --global core.autocrlf true
gh auth setup-git
gh auth status
```

`gh auth status` "Logged in to github.com account <kullanıcı adı>" demeli.

### B3. Deneme: repoyu indir, LFS'i dene

```powershell
New-Item -ItemType Directory -Force C:\Dev | Out-Null
cd C:\Dev
$env:GIT_LFS_SKIP_SMUDGE = "1"
git clone https://github.com/Andacs/Sallanti Sallanti_deneme
Remove-Item Env:GIT_LFS_SKIP_SMUDGE
cd Sallanti_deneme
git lfs pull --include "Art/Audio/MUS_Cozy.wav"
(Get-Item Art\Audio\MUS_Cozy.wav).Length
```

Son satır **10240044** (yaklaşık 10 MB) yazmalı. 130–200 gibi küçük bir sayıysa LFS çalışmıyor:
`git lfs install` ve tekrar `git lfs pull --include "Art/Audio/MUS_Cozy.wav"`.

Deneme bitince klasörü silmek isteyip istemediğini insana sor (`C:\Dev\Sallanti_deneme`, jam'de gerekmez).

### B4. Kontrol tablosu

Sohbete şu tabloyu yaz:

```
GITHUB KURULUM RAPORU — <bilgisayar adı> — <tarih saat>
git        : <sürüm>
git lfs    : <sürüm>
gh         : <sürüm>, giriş: <kullanıcı adı>
git kimliği: <ad> <e-posta>
Deneme klonu + LFS dosyası (10240044 bayt): TAMAM / BOZUK
SONUC: GITHUB HAZIR  (ya da: eksik ...)
```

İnsan bu raporu Andaç'a gönderir.

---

## C. Jam başladıktan sonra (16 Ekim akşamı): repoya katıl ve model gönder

Andaç repo adını (`Andacs/<Ad>`) söyleyip davet gönderdikten ve insan daveti kabul ettikten sonra:

```powershell
gh api repos/Andacs/<Ad> -q .permissions.push
```

`true` yazmalı (yazmıyorsa davet kabul edilmemiş). Sonra:

```powershell
cd C:\Dev
git clone https://github.com/Andacs/<Ad>
```

### Model teslim kuralları

- Modelcinin dosyaları **yalnızca `Art/Modeller/`** altına girer:
  - `Art/Modeller/<Kategori>/SM_<Ad>.fbx`: Unreal'e girecek mesh (ör. `Art/Modeller/Bina/SM_Bina_Ev.fbx`)
  - `Art/Modeller/<Kategori>/T_<Ad>_<Tür>.png`: dokular (`_D` renk, `_N` normal, `_ORM`)
  - `Art/Modeller/<Kategori>/<Ad>.blend`: kaynak dosya
- **İsimde Türkçe karakter, boşluk yok** (`SM_Bina_Kutuphane`, `kütüphane` değil).
- **`Content/`, `Source/`, `Config/` ve başka hiçbir klasöre dokunma.** `.uasset` / `.umap` dosyalarını
  Unreal'de programcı oluşturur.
- Ölçü, pivot, FBX ayarları: repodaki `Docs/JamKit/08_ASSET_SPEC.md` (§10 FBX export) ve
  `Docs/JamKit/07_BLENDER_UNREAL.md`.

### Model gönderme (her teslimde)

```powershell
cd C:\Dev\<Ad>
git pull
# dosyaları Art\Modeller\<Kategori>\ altına kopyala
git add Art/Modeller
git status
git commit -m "Model: SM_Bina_Ev (<isim>)"
git push
git lfs ls-files | Select-String "SM_Bina_Ev"
```

- `git status`'ta **yalnızca `Art/Modeller/` altındaki** dosyalar görünmeli. Başka bir şey varsa commit
  etme, insana söyle.
- Son satır dosyayı listelemeli (LFS'e girdi demek).
- `git push` reddedilirse (`rejected`, `fetch first`): `git pull --rebase`, sonra tekrar `git push`.
  Çakışma (conflict) çıkarsa **dur**, hiçbir şeyi silme, Andaç'a haber ver.
- **Asla** `git push --force`, `git reset --hard`, `git clean` kullanma.
- Gönderince Andaç'a dosya adını yaz: "SM_Bina_Ev push edildi".
