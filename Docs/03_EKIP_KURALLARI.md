# Ekip kuralları — Unreal + binary asset + 48 saat

Unreal'de ekiple çalışmanın temel gerçeği: **`.umap` ve `.uasset` merge edilemez.** İki kişi aynı
seviyeyi değiştirirse biri kaybeder, ve Git bunu "çakışma" diye bildirmez — binary dosyada
"ikisinden birini seç" der. 48 saatte bu bir günlük iş demek.

Aşağıdaki kurallar bunun etrafından dolaşmak için. Jam başında **birlikte okunacak**, 15 dakika.

---

## 1. Seviye dosyasının tek bir sahibi var

Tek seviye, tek sahip. O kişi dışında kimse seviyeyi kaydetmez.

Başkası bir şey yerleştirmek istiyorsa iki yol var:
- Sahibe söyler, sahip yerleştirir
- Sahip "seviye sende" der, dosyayı bırakır, öteki alır, bitirince geri verir

**Sözlü devir yeterli ama açık olmalı:** "seviye bende" / "seviye sende". İkisi birden "bende"
sanıyorsa iş kaybedilmiştir.

> Jam'de Perforce kurmaya değmez; disiplin yeterli. Ama disiplin olmazsa Git seni korumaz.

## 2. Seviyeyi kopyalayıp "onda çalışmak" YASAK

Bu, SonZarf'ın en pahalı ekip hatası.

> 3 Ağustos'ta `map_village_day21`'in bir kopyası `L_Village_Cozy` olarak açıldı ve "iş onda
> yapılsın" dendi. Üç hafta sonra: hikâye hâlâ day21'i yüklüyordu, bütün iş cozy'de birikmişti ve
> **106 aktör yanlış dosyada kalmıştı** — ilan tahtası, ocak, 6 ağaç, 73 hayvan, 25 NPC.
>
> Taşımak için üç yol denendi, üçü de kapandı: `duplicate_actor` editörü çökertti, `move_actors_to_level`
> sessizce 0 döndürdü, seçim tabanlı yol commandlet'te seçim olmadığı için hiç çalışmadı. Sonunda
> elle kopyala-yapıştır kaldı.
>
> Üstüne: "day21 toptan cozy olsun" demek de olmadı, çünkü ölçünce day21'de cozy'de **olmayan 67
> aktör** çıktı (ışık blueprint'leri) — toptan değiştirme onları silerdi.

Ders: **iki seviye asla paralel yaşamaz.** Deneme yapılacaksa ayrı bir *test* seviyesi açılır ve
oraya konan hiçbir şey ana seviyeye taşınmayacağı bilinerek konur.

## 3. Pull veya branch değiştirmeden önce editör kapanır

- [ ] Editörü kapat
- [ ] `git pull`
- [ ] Editörü aç

> Açık editör `.uasset`/`.umap` dosya kilidi tutuyor. SonZarf'ta bu bir kez `git checkout`'u yarım
> uyguladı: kaynak dosyalar silindi, binary'ler silinmedi. Kurtarılabildi (`git reset --mixed` +
> `git checkout -- .`) ama yarım saat yedi ve veri kaybı riski gerçekti.

## 4. Kim neyin sahibi

Jam başında doldurulacak. Örtüşme olmayacak:

```
Kod / oynanış        : ___________
Seviye dosyası       : ___________   <- tek kişi
Asset / sanat        : ___________
Ses                  : ___________
Paketleme / teslim   : ___________
```

**Sahiplik dışına çıkılmaz.** Bir kod dosyasına iki kişi dokunacaksa önce konuşulur — `.cpp` merge
edilebilir ama 48 saatte merge çözmek de zaman.

## 5. Commit ve push ritmi

- **Çalışan her şey commit edilir.** "Sonra toparlarım" yok.
- **Her commit push edilir.** Yerelde bekleyen commit yedeklenmemiş iştir.
- Commit mesajı tek satır, ne yaptığını söyler: `feat: ışık tükenme mekaniği`

> SonZarf'ta çalışma üç hafta tek diskte durdu: `develop`'tan 101 commit ileri, origin'e 69 commit
> gönderilmemiş. Tek disk arızası üç haftayı götürürdü. 48 saatte aynı hata jam'i götürür.

Dal yapısı 48 saatte basit tutulur: **tek dal (`main`), herkes oraya push eder.** Feature branch +
PR akışı jam'de maliyeti faydasından fazla.

## 6. Asset içe aktarma kuralları

- Yeni asset **`Content/<ProjeAdi>/`** altına. Vendor klasörüne dokunulmaz.
- **Mevcut bir asset'in üstüne import yapılmaz** — import ayarları sadece *yeni yaratılan* varlığa
  uygulanır, mevcut varlık eski ayarlarını korur. Değiştirmek gerekiyorsa önce sil, sonra import et.
- Asset yolu **ezberden yazılmaz.** SonZarf'ta `SM_PROP_chest_01` diye bir mesh olduğu varsayıldı,
  yoktu; kayıt sandığı görünmez olacaktı.

## 7. "Bende çalışıyor" yasak cümlesi

Bir şey bittiğinde **başka birinin makinesinde** çalıştığı görülecek. Jam'de tek makinede çalışan
şey teslim edilemez.

Pratikte: 24. ve 36. saatte herkes temiz klon alıp derler ve oynar. İki kontrol noktası, 10 dakika
her biri.

---

## Çakışma olduğunda

`.uasset`/`.umap` çakışmasında merge yoktur, seçim vardır:

```bash
git checkout --theirs Content/yol/Dosya.umap   # uzaktakini al
git checkout --ours   Content/yol/Dosya.umap   # kendindekini al
```

Hangisini seçeceğine **seviye sahibi** karar verir. Seçilmeyen taraftaki iş kaybedilir — bu yüzden
1. kural var.
