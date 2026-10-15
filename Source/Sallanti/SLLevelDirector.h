// Bolum yoneticisi: gorev listesi, slot kilitleri, ailelerin gelisi ve mutlulugu, yol baglantisi,
// otobus, bolum sonu. Gorevler ve aileler veridir (EditAnywhere); ConfigureLevel(N) N. bolumun
// varsayilanlarini yazar. Seviyede bir tane varsa o kullanilir; yoksa GameMode dogurur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLLevelDirector.generated.h"

class ASLBus;
class ASLFamily;
class ASLPlaceable;
class ASLPlatform;
class USLBuildComponent;

UENUM(BlueprintType)
enum class ESLTaskKind : uint8
{
	PlaceType,     // TypeId'den Count tane yerlestir
	FamilyMovedIn, // TypeId binalarindan Count tanesine aile yerlessin
	ConnectRoads,  // ConnectTypes'in her birinden biri ayni yol agina degsin
	FamiliesHappy  // butun ailelerin butun istekleri karsilansin
};

USTRUCT(BlueprintType)
struct FSLTask
{
	GENERATED_BODY()

	/** Ekranda gorunen gorev cumlesi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Text;

	/** Ailenin konusma balonu: gorevi aile verir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Speech;

	/** Gorev bitince ailenin kisa tesekkuru (bos = yok). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString DoneSpeech;

	/** Kucuk ipucu satiri (bos olabilir). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Hint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	ESLTaskKind Kind = ESLTaskKind::PlaceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FName TypeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	int32 Count = 1;

	/** ConnectRoads: birbirine baglanacak turler. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TArray<FName> ConnectTypes;

	/** Bu gorev BASLAYINCA acilan slotlar (tur kimligiyle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TArray<FName> Unlocks;

	/** Konusan aile (Families indeksi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	int32 Speaker = 0;
};

UENUM(BlueprintType)
enum class ESLNeedKind : uint8
{
	ConnectedTo, // evin kapisindan giden yol agi Type'a degiyor
	NearType,    // evden en yakin Type binasina uzaklik <= MaxCells (hucre, sehir blogu: |dx|+|dy|)
	FarFrom,     // evden en yakin Type binasina uzaklik >= MinCells (gurultu: "AVM'den uzak"); Type yoksa tamam
	NearFamily   // FamilyIndex ailesinin evine uzaklik <= MaxCells (akrabalar yan yana)
};

/** Bir ailenin istegi. Mutluluk = isteklerin ortalamasi; hepsi tamamsa aile mutlu. */
USTRUCT(BlueprintType)
struct FSLNeed
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	ESLNeedKind Kind = ESLNeedKind::ConnectedTo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FName Type;

	/** NearType: en fazla bu kadar hucre uzak. 2. bolum: 3 (Andac, 15 Eki). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti", meta = (ClampMin = "0"))
	int32 MaxCells = 4;

	/** FarFrom: en az bu kadar hucre uzak. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti", meta = (ClampMin = "0"))
	int32 MinCells = 4;

	/** NearFamily: hangi aileye yakin (Families indeksi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	int32 FamilyIndex = 0;

	/** HUD'da istegin kisa adi, ornegin "Durağa yakın". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Label;

	/** Karsilanmazken ailenin sikayeti (balon; istek bozulunca da soylenir). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Complaint;

	/** Istek karsilandigi an ailenin sevinci (balon). Bos = sessiz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString MetLine;
};

USTRUCT(BlueprintType)
struct FSLFamilyDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Name = TEXT("Aile");

	/** Gelince bekledigi yer (platform yerelinde, cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FVector WaitLocal = FVector(-230.f, -250.f, 0.f);

	/** Bu gorev baslayinca platformda belirir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	int32 ArriveAtTask = 0;

	/** Otobusle gelir: duragin onunde belirir (durak yoksa WaitLocal'da). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	bool bArriveAtStop = false;

	/** Bu turden bir bina konunca gelir (bos = ArriveAtTask'a bakar). Ornek: Durak -> otobusle gelen aile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FName ArriveWhenType;

	/** Istekler gizli: oyuncu aileye tiklayinca ogrenir ("Is This Seat Taken?"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	bool bWishesHidden = false;

	/** Tiklaninca soyledigi tanisma cumlesi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Greeting;

	/** Mutluyken ara ara soyledikleri. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TArray<FString> HappyLines;

	/** Ev beklerken soyledikleri. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TArray<FString> WaitLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FLinearColor Tint = FLinearColor(0.15f, 0.35f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TArray<FSLNeed> Needs;
};

UCLASS()
class SALLANTI_API ASLLevelDirector : public AActor
{
	GENERATED_BODY()

public:
	ASLLevelDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** N. bolumun varsayilan gorevlerini ve ailelerini yazar (BeginPlay'den once cagrilir). */
	void ConfigureLevel(int32 Number);
	void ConfigureLevel3();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	int32 LevelNumber = 1;

	/** Bolum bitince Enter bu bolume gecer. 0 = son bolum. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	int32 NextLevelNumber = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	FString LevelTitle = TEXT("1. Bölüm: İlk mahalle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	TArray<FSLTask> Tasks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	TArray<FSLFamilyDef> Families;

	/** Ailesi olmayan yeni eve kenardan komsu gelsin mi (1. bolum). Kapaliysa ev bos kalir; sonra gelen
	 *  aile bos eve yerlesir (2. bolum: ikinci aile oyuncunun kurdugu eve). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	bool bNeighborsArrive = true;

	/** Bolum sonu icin platformun dengede olmasi gerekir mi? (devriliyor durumunda bitmez) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	bool bRequireBalanceToComplete = true;

	/** Acik bolum (16 Eki, Andac: "gorevler sirayla, simulasyon gibi"): sirali gorev yok, tek hedef
	 *  (herkes mutlu + RequiredTypes kurulu). Binalar bastan acik ama stoklu (SlotLimits). Aile seciliyken
	 *  ev kurmak ya da bos eve tiklamak aileyi oraya tasir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	bool bOpenLevel = false;

	/** Acik bolumde bitis icin kurulmus olmasi gereken turler (mahalle plani). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	TArray<FName> RequiredTypes;

	/** Bolumun stoklari (BuildComponent::Limits'e yazilir). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	TMap<FName, int32> SlotLimits;

	/** Yeni eve bekleyen ilk aile kendiliginden tasinir (1. bolum). Kapaliysa oyuncu secer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	bool bAutoAssign = true;

	/** Ailelerin kendiliginden konusma araligi, sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	FVector2D ChatterSeconds = FVector2D(9.f, 16.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float BubbleSeconds = 3.2f;

	/** Bolum bitince son soz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	FString FinalSpeech = TEXT("Mahallemiz hazır. Teşekkürler başkan!");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	float DoneSpeechSeconds = 2.5f;

	// --- Zorluk (bolume gore platforma yazilir; 1. bolum egitim: platformun kendi degerleri) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zorluk")
	bool bApplyTuning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zorluk")
	float TunePlatformMass = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zorluk")
	float TuneTiltPerCm = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zorluk")
	float TuneFailSeconds = 1.5f;

	/** Hamle siniri (0 = yok). Dolunca yapi kapanir; bolum bitmediyse "Süre doldu". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zorluk", meta = (ClampMin = "0"))
	int32 MoveLimit = 0;

	// --- Gece-gunduz: her hamle saati ilerletir ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zaman")
	float StartHour = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zaman", meta = (ClampMin = "0"))
	float HoursPerMove = 1.f;

	int32 GetMoveCount() const;
	/** Bolum basindan beri gecen saat dahil mutlak saat (7 = 1. gun 07:00, 31 = 2. gun 07:00). */
	float GetTimeHours() const { return StartHour + GetMoveCount() * HoursPerMove; }
	int32 GetMovesLeft() const { return MoveLimit > 0 ? FMath::Max(0, MoveLimit - GetMoveCount()) : -1; }

	/** Otobus: bu turden bir bina (durak) yol agina bagliysa otobus o agda gidip gelir. Bos = otobus yok. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus")
	FName BusStopType;

	/** Elde ev varken duraga yakin bolgenin rengi. Turkuaz nane karoda kayboluyordu (ekran goruntusu): sicak sari. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus")
	FLinearColor NearZoneColor = FLinearColor(1.f, 0.86f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus", meta = (ClampMin = "0", ClampMax = "1"))
	float NearZoneOpacity = 0.8f;

	/** HUD'daki mutluluk barinin yaklasma hizi (1/sn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float HappinessLerpSpeed = 2.f;

	// --- Durum (HUD okur) ---

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 CurrentTask = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bLevelComplete = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 Progress = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 ProgressGoal = 1;

	bool IsTaskDone(int32 Index) const { return Index < CurrentTask || bLevelComplete; }

	/** Yol agina degen hedef turu sayisi (HUD ve test icin). */
	int32 CountConnected(const TArray<FName>& Types) const;

	/** Su an gosterilecek konusma balonu. false = balon yok. */
	bool GetSpeech(FString& OutText, FVector& OutWorldAnchor) const;

	// --- Aileler (HUD) ---

	/** Bir ailenin istegi var mi (yoksa HUD mutluluk kartini cizmez). */
	bool HasFamilyNeeds() const;
	/** Aile platforma geldi mi. */
	bool IsFamilyPresent(int32 I) const;
	/** 0..1, isteklerin ortalamasi. Evi yoksa 0. */
	float GetHappiness(int32 I) const { return Happiness.IsValidIndex(I) ? Happiness[I] : 0.f; }
	float GetDisplayHappiness(int32 I) const { return DisplayHappiness.IsValidIndex(I) ? DisplayHappiness[I] : 0.f; }
	bool IsFamilyHappy(int32 I) const;
	/** Tek satir durum: "Mutlu" ya da karsilanmayan ilk istek ("Durak 5 birim uzakta, en fazla 3"). */
	FString GetFamilyStatus(int32 I) const;
	ASLPlaceable* GetHome(int32 I) const { return Homes.IsValidIndex(I) ? Homes[I].Get() : nullptr; }

	/** Elde ev varken: buraya konursa (henuz evi olmayan / tasinan evin) ailenin NearType istegi. */
	bool DescribeHomePlacement(const ASLPlaceable* Ghost, const ASLPlaceable* Moving, FString& Out, bool& bGood) const;

	ASLBus* GetBus() const { return Bus; }

	// --- Aileyle etkilesim (HUD ve PlayerController cagirir) ---

	/** Aileye tiklandi: istekleri acilir, secilir, ziplar, tanisma cumlesini soyler. */
	void SelectFamily(int32 I);
	void ClearFamilySelection() { SelectedFamily = INDEX_NONE; }
	int32 GetSelectedFamily() const { return SelectedFamily; }
	void SetHoveredFamily(int32 I) { HoveredFamily = I; }
	bool IsFamilyDiscovered(int32 I) const { return Discovered.IsValidIndex(I) && Discovered[I]; }
	/** Aileyi bu eve tasi (bos olmali). Eski evi bosalir. */
	bool AssignFamilyToHouse(int32 I, ASLPlaceable* House);
	ASLFamily* GetFamilyActor(int32 I) const { return FamilyActors.IsValidIndex(I) ? FamilyActors[I].Get() : nullptr; }
	/** Ailenin su anki konusma balonu (kendiliginden konusma, tepki). */
	bool GetFamilyBubble(int32 I, FString& Out) const;
	/** Istek su an karsilaniyor mu (HUD istek karti). */
	bool IsNeedMet(int32 Fam, int32 Need) const;
	bool IsRequiredPlaced(FName Type) const;
	/** Acik bolum: hedefe kalan (mutsuz aile sayisi + eksik plan). */
	FString DescribeOpenGoal() const;

private:
	ASLPlatform* GetPlatform() const;
	USLBuildComponent* GetBuild() const;
	void ApplyUnlocks();
	void StartFamilies();
	bool EvaluateTask(const FSLTask& Task, int32& OutProgress, int32& OutGoal) const;
	void CompleteLevel();
	void SpawnFamilies();
	void UpdateHappiness(float DeltaSeconds);
	void UpdateBus();
	void UpdateNearZone();
	/** 0..1. OutDetail: karsilanmadiysa kisa aciklama. Cell/Foot verilirse ev oradaymis gibi. */
	float EvaluateNeed(const FSLNeed& Need, const ASLPlaceable* Home, FString* OutDetail, const ASLPlaceable* Ignore = nullptr) const;
	/** Ev ayak izinden en yakin Type binasina sehir blogu uzakligi; yoksa -1. */
	int32 DistanceToType(FIntPoint CellMin, FIntPoint Foot, FName Type, const ASLPlaceable* Ignore) const;
	int32 FindFamilyOfHome(const ASLPlaceable* House) const;
	/** Konusan ailenin balon capasi. */
	bool GetFamilyAnchor(int32 I, FVector& Out) const;

	/** Kapisina yol degen binalar: yeni baglananda efekt (bina halkasi + yol boyunca akan parlama). */
	void UpdateConnectionFx();

	TSet<TWeakObjectPtr<ASLPlaceable>> ConnectedBuildings; // zayif isaretci: GC'ye UPROPERTY gerekmez

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASLFamily>> FamilyActors;

	TArray<TWeakObjectPtr<ASLPlaceable>> Homes;
	TArray<float> Happiness;
	TArray<float> DisplayHappiness;

	UPROPERTY(Transient)
	TObjectPtr<ASLBus> Bus;

	FString DoneText;
	int32 DoneSpeaker = 0;
	FString LastSpeech;
	float DoneUntil = -1.f;

	int32 UnlockedForTask = -1;
	float OutOfMovesTimer = 0.f;

	TArray<bool> Discovered;
	TArray<TArray<bool>> NeedMet;
	TArray<bool> NeedInit;
	TArray<FString> BubbleText;
	TArray<float> BubbleUntil;
	TArray<float> NextChat;
	int32 SelectedFamily = INDEX_NONE;
	int32 HoveredFamily = INDEX_NONE;
	bool bWasNight = false;
	bool bIntroOver = false;

	void Say(int32 I, const FString& Text, float Seconds = -1.f);
	void UpdateChatter(float DeltaSeconds);
	void ConfigureLevel2();

	void ApplyTuning();
	void CheckMoveLimit(float DeltaSeconds);
	/** Iki ayak izi arasi sehir blogu uzakligi (komsu = 1). */
	static int32 FootDistance(FIntPoint MinA, FIntPoint FootA, FIntPoint MinB, FIntPoint FootB);
	/** Ailenin uzaklik isteklerinin hepsi bu hucrede (1x1 ev) tutuyor mu; uzaklik istegi yoksa false. */
	bool CellSatisfiesDistanceNeeds(int32 Fam, FIntPoint Cell, const ASLPlaceable* Ignore) const;
	int32 FamilyForPlacement(const ASLPlaceable* Moving) const;
};
