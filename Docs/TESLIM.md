# Teslim hazırlığı (CIVIL GAME JAM 2026, kural kitapçığı v09)

Takım temsilcisi bu sayfadan forma kopyalar. Son saat: **18 Ekim 2026 Pazar 18.00 (UTC+3)**, yükleme
kaydı sunucuda o saatten önce oluşmalı. Git desteği için sınır 12.00. Teslim ekranı:
https://game.ktimo.com.tr/jam/teslim

> **Önemli (madde 3.1):** Sallantı 14–15 Ekim'de, tema açıklanmadan yapıldı; teslim edilemez. Bu sayfa ve
> araçlar jam oyununa aynen uygulanacak düzen. Jam'de değişecek alanlar [köşeli parantezde].

## Formdaki alanlar (madde 5.2)

| Alan | Değer |
|---|---|
| Takım | FreakLabs |
| Oyun adı | [Sallantı] |
| Kısa açıklama | [Sallanan bir adaya şehir kur, aileleri mutlu et, dengeyi koru. Her bina adanın ağırlık merkezini kaydırır; ailelerin istekleri birbirini zorlar.] |
| Platform | Windows |
| Oynanabilir sürüm | `Build/Teslim/Sallanti_Windows.zip` (Shipping, 348 MB; repoya girmez). `bash Tools/teslim_hazirla.sh` üretir ve dener: 15 Ekim 21.26'da commit `e8b7b78`'den paketlendi, temiz klasöre açıldı, ana menü ekranda görüldü (`Build/Teslim/deneme_ekrani.png`). Tamamı 3 dk 22 sn. **Not:** ilk elle yapılan ZIP (682 MB) aslında tar'dı, Windows açamazdı; betik artık `PK` imzasını kontrol ediyor. |
| Video (≤ 1 dk, MP4) | `Build/Teslim/Sallanti_Trailer.mp4` (36,5 sn, 1920×1080, H.264 + AAC) |
| Public repo | [https://github.com/Andacs/Sallanti] public (15 Ekim, girişsiz açıldığı denendi). Jam oyununun reposu da teslimden önce public olmalı (madde 6.5, aksi hâlde diskalifiye). |
| Kurulum / çalıştırma / kontroller | `README.md` → "Çalıştırma" bölümü (EK A.3 şablonu) |
| Hazır kaynak beyanı | `README.md` → "Hazır kaynak beyanı" (beş kategori) |
| AI beyanı | `README.md` → "Yapay zekâ beyanı" (beş kategori) |
| Kural kabulü | Temsilci; ekip hak sahiplerinin onayı alınmış olmalı (madde 9.8) |

## Teslim kontrol listesi (kitapçık son sayfa)

| Madde | Durum |
|---|---|
| Takım 2–4 kişi | 4 kişi (Andaç, Yiğit, Mert, Asil) |
| Oyun 48 saatte hazırlandı | **Sallantı için HAYIR** (prova). Jam oyunu için 16 Ekim 18.00 sonrası commit'ler kanıt. |
| Build test edildi, ZIP hazır | `bash Tools/teslim_hazirla.sh` (paket, ZIP, temiz klasörde deneme, video, repo, beyanlar; sonuç `Build/Teslim/TESLIM_OZET.txt`) |
| Video ≤ 1 dk MP4 | `bash Tools/trailer_yap.sh` |
| Repo public, doğru bağlantı | Sallantı: tamam. Jam oyunu: public yap, tarayıcıda girişsiz aç, gizli bilgi taraması yap (`git grep -niE "token\|secret\|password\|apikey"`) |
| ZIP ile repo uyumlu | ZIP'i son commit'ten üret, commit hash'ini not et |
| Hazır kaynak beyanı (5 kategori) | README'de |
| Kısıtlı ham asset repoda yok | Yok (motor içerikleri motorla gelir) |
| Kurulum, kontroller yazıldı | README'de |
| AI beyanı (5 kategori) | README'de; oyun içi jenerikte de aynı içerik |
| 3 dk sunuma hazırlık | Trailer arkada oynar, anlatım 3 dk (soru-cevap yok) |
| Hak sahiplerinin onayı | Ekipten alınacak |
| Makbuz kontrolü | Yüklemeden sonra makbuz no, sürüm, saat, dosya adları |

## Oyun içinde kurallarla ilgili olanlar

- **Açılış videosu yok** (23 Ekim): oyun doğrudan ana menüyle açılır.
- **Ana menü:** Oyna · Bölümler · Nasıl oynanır · Emeği geçenler · Çıkış. Arkada canlı, dönen kasaba.
- **Jenerik (Emeği geçenler):** ekip, hazır kaynaklar, yapay zekâ kullanımı, teşekkürler. README'deki
  beyanlarla aynı içerik (madde 3.4 ve 4.2 "kaynaklar takımın özgün üretimi gibi gösterilemez").
  Takım adı ve ekip satırları: `ASLHUD::TeamName`, `ASLHUD::TeamMembers`.
- Son bölüm bitince **Emeği geçenler** düğmesi jeneriği açar.
