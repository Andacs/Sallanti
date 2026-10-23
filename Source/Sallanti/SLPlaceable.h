// Yerlestirilen nesnenin tabani. Pivot tabanin ortasinda (08_ASSET_SPEC 3. bolum).
// Hem gercek nesne hem hayalet olarak kullanilir; hayalet collision'siz ve yari saydamdir.
// Yerlestirilen nesne platforma bagli ve sabittir (fizik yok); platform devrilince onunla devrilir.
//
// Gorunus (14 Ekim, Blender taslagi): bina gri bir PLAKA uzerinde durur; plaka ayak izini derzler dahil
// kaplar, govde plakadan icerde. Yol (bFlat) plakasiz tek karo; komsu yollara ve kapisi kendine bakan
// binaya dogru derzi kapatan uzantilar cikarir, boylece yollar birlesir.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLPlaceable.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS()
class SALLANTI_API ASLPlaceable : public AActor
{
	GENERATED_BODY()

public:
	ASLPlaceable();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Bina govdesi ya da yol karosu. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Binanin altindaki gri plaka (yolda gizli). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UStaticMeshComponent> Plate;

	/** Tur kimligi: gorevler buna bakar (Ev, Hastane, AVM, Yol). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FName TypeId;

	/** Dengeye katki agirligi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float Mass = 1.f;

	/** Izgara adimi, cm. Asset spec'iyle ayni: 50. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float GridSize = 50.f;

	/** Karolar arasi derz, cm (platformdan gelir). Plaka/yol = ayak izi - derz: karoyla hizali. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float TileGap = 8.f;

	/** Boyut, modul cinsinden (G x D x Y). 1 modul = GridSize cm. Duz nesnede Y yok sayilir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FIntVector SizeCells = FIntVector(1, 1, 1);

	/** Duz nesne (yol): yuksekligi FlatHeightCm, ustune bir sey konmaz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	bool bFlat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float FlatHeightCm = 2.5f;

	/** Ust uste binen yol parcalari arasi yukseklik farki, cm (z-fighting). 0.2 cm parca kenarlarinda ince
	 *  cizgi birakiyordu (ekran goruntusu); ters-Z derinlikte 0.05 yetiyor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yol", meta = (ClampMin = "0.01"))
	float RoadZStepCm = 0.05f;

	/** Plaka kalinligi, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float PlateHeightCm = 2.f;

	/** Govdenin plaka kenarindan iceri payi, cm (her yandan). 5'ten 8'e: plaka-bina arasi daha genis (14 Eki). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float BodyInsetCm = 8.f;

	/** Kapidan yola uzanan giris yolu (kaldirim) rengi; yalnizca bina yola bagliyken gorunur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	FLinearColor EntrancePathColor = FLinearColor(0.99f, 0.95f, 0.86f);

	/** Viraj kac parcayla cizilir (cok = daha yuvarlak). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yol", meta = (ClampMin = "2", ClampMax = "16"))
	int32 CurveSegments = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FLinearColor Tint = FLinearColor(0.62f, 0.45f, 0.28f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FLinearColor PlateColor = FLinearColor(0.97f, 0.93f, 0.86f);

	/** Kapisi var mi? Kapi yerel +X yuzunde; yol ancak kapinin onundeki hucreye degerse baglanir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	bool bHasEntrance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	FLinearColor DoorColor = FLinearColor(0.55f, 0.33f, 0.24f);

	/** Kapi onu isareti (yerlestirirken gorunur). Yol ustune gelince ortulur: baglandigi gorulur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	FLinearColor EntranceMarkColor = FLinearColor(0.95f, 0.75f, 0.15f);

	// --- Cozy suslemesi (14 Eki): DisplayMesh yokken TypeId'ye gore cati, pencere, haç, tabela, agac ---

	/** Kapali = yalniz govde kutusu (eski gorunus). Mesh gelince zaten cizilmez. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	bool bDecor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor RoofColor = FLinearColor(0.86f, 0.33f, 0.28f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor WindowColor = FLinearColor(0.38f, 0.58f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor TreeColor = FLinearColor(0.20f, 0.45f, 0.38f);

	/** Pencereler isiksiz (gece yanar). Gunduz rengi = WindowColor * WindowDayBrightness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	float WindowDayBrightness = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor WindowNightColor = FLinearColor(1.f, 0.78f, 0.38f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	float WindowNightBrightness = 3.f;

	/** 0 gunduz .. 1 gece: pencereler sicak sariya doner (gece-gunduz, ASLAmbience cagirir). */
	void SetNightGlow(float Night);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor BushColor = FLinearColor(0.38f, 0.66f, 0.42f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor AccentColor = FLinearColor(0.30f, 0.72f, 0.70f);

	// --- Cozy ayrintilar (23 Eki): pencere cercevesi, panjur, cicek kutusu, cizgili tente, fener ---

	/** Pencere cercevesi, kayitlar, cati kornisi: krem beyaz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor TrimColor = FLinearColor(1.f, 0.97f, 0.90f);

	/** Ev panjurlari. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor ShutterColor = FLinearColor(0.42f, 0.66f, 0.62f);

	/** Cicek kutusu ve saksi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor PlanterColor = FLinearColor(0.78f, 0.52f, 0.38f);

	/** Hastanenin besik catisi (evinki RoofColor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor HospitalRoofColor = FLinearColor(0.45f, 0.70f, 0.72f);

	/** Cicekler: kutularda ve saksilarda sirayla. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	TArray<FLinearColor> FlowerColors = { FLinearColor(0.98f, 0.55f, 0.60f), FLinearColor(1.f, 0.85f, 0.35f), FLinearColor(0.80f, 0.60f, 0.95f) };

	/** AVM dukkan tenteleri: her dukkan bir renk, krem ile serit serit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	TArray<FLinearColor> AwningColors = { FLinearColor(0.93f, 0.45f, 0.42f), FLinearColor(0.30f, 0.72f, 0.70f), FLinearColor(0.98f, 0.78f, 0.35f) };

	/** Tente seridi sayisi (dukkan basina). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme", meta = (ClampMin = "2", ClampMax = "12"))
	int32 AwningStripes = 6;

	/** Fener ve direk rengi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Susleme")
	FLinearColor LampPostColor = FLinearColor(0.25f, 0.38f, 0.36f);

	/** Bos = olcekli gri kup. Modelcinin mesh'i verilirse olceklenmez; kapisi mesh'te, kod kapi cizmez. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	TObjectPtr<UStaticMesh> DisplayMesh;

	// --- Baglanti efekti ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Efekt")
	FLinearColor ConnectFxColor = FLinearColor(0.35f, 1.f, 0.45f);

	/** Efekt suresi, sn: halka yayilir ve solar, plaka parlayip soner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Efekt")
	float ConnectFxSeconds = 0.8f;

	/** Bina yola baglaninca (ya da yol akisi buraya gelince). Delay: yol boyunca sirayla parlasin diye. */
	void PlayConnectFx(float DelaySeconds = 0.f, bool bRing = true);

	/** Yol: hangi yonlere uzanti (bit 0 +X, 1 +Y, 2 -X, 3 -Y; platform hucre ekseni).
	 *  Iki dik yon = viraj (kavisli), tek yon = cikmaz (yuvarlak uc), digerleri kare karo. */
	void SetRoadLinks(uint8 Mask);

	/** Bina: kapisina yol degiyor mu? Degiyorsa kapidan yola giris yolu gorunur. */
	void SetEntranceConnected(bool bConnected);
	bool IsEntranceConnected() const { return bEntranceConnected; }
	uint8 GetRoadLinks() const { return RoadLinkMask; }

	// --- Aile ---

	/** Bu binaya tasinacak kisi sayisi (Ev: aile). 0 = kimse gelmez. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	int32 Residents = 0;

	/** Aile uyesinin boyu, cm. Spec 160 diyordu; ev 50 cm'e inince oran icin gecici 35. Karar Asil'de. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float ResidentHeightCm = 35.f;

	/** Yurume hizi, cm/sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float ResidentWalkSpeed = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	FLinearColor ResidentTint = FLinearColor(0.15f, 0.35f, 0.75f);

	// --- Platform uzerindeki yeri (BuildComponent yazar, platform okur) ---

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	FIntPoint CellMin = FIntPoint::ZeroValue;

	/** Tabanin bulundugu yukseklik, modul cinsinden. 0 = platform yuzeyi. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 Level = 0;

	/** Bakis yonu: 0 = kapi +X, 1 = +Y, 2 = -X, 3 = -Y (platform yerelinde). Yaw = Facing * 90. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 Facing = 0;

	/** Oyuncunun elinde (tasiniyor): gizli, dengeye ve hucre doluluguna katilmaz. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bHeld = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bResidentsArrived = false;

	bool HasResidentsStarted() const { return bResidentsStarted; }

	/** Aile aktoru (ASLFamily) bu eve atandi / vardi. Evin kendi figurleri dogmaz; aile kapinin onunde durur. */
	void MarkFamilyAssigned(bool bAssigned) { bResidentsStarted = bAssigned; if (!bAssigned) { bResidentsArrived = false; } }
	void MarkFamilyArrived() { bResidentsStarted = true; bResidentsArrived = true; }

	/** Platform yerelinde: aile buradan kapinin onune nasil yurur (binanin icinden gecmez). Son nokta kapi onu. */
	TArray<FVector> GetArrivalPath(const FVector& StartPlatformLocal) const;
	/** Platform yerelinde: ailenin evde durdugu yer (kapinin onu, giris yolunun yani) ve yuzu. */
	FVector GetDoorSpotLocal() const;
	float GetDoorSpotYaw() const { return 90.f * Facing; }

	/** Konunca kisa "boing": %88 -> %106 -> %100 (sifir olcek yok). */
	void PlayPop();

	/** Platform uzerindeki XY ayak izi, donmeyi hesaba katarak. */
	FIntPoint GetFootprint() const { return (Facing & 1) ? FIntPoint(SizeCells.Y, SizeCells.X) : FIntPoint(SizeCells.X, SizeCells.Y); }

	/** Kapinin baktigi yon, platform hucre ekseninde: (1,0) (0,1) (-1,0) (0,-1). */
	FIntPoint GetFacingDir() const;

	/** Kapinin onundeki hucre(ler): yuzun ortasi; cift genislikte ortadaki iki hucre. Kapi yoksa bos. */
	TArray<FIntPoint> GetEntranceCells() const;

	/** Kapi onu isaretlerini goster/gizle (yerlestirme sirasinda). */
	void SetEntranceMarkersVisible(bool bVisible);

	/** Ustune konabilecek yukseklik, modul cinsinden. Duz nesne 0. */
	int32 GetStackHeight() const { return bFlat ? 0 : SizeCells.Z; }

	bool CoversCell(const FIntPoint& Cell) const
	{
		const FIntPoint F = GetFootprint();
		return Cell.X >= CellMin.X && Cell.Y >= CellMin.Y && Cell.X < CellMin.X + F.X && Cell.Y < CellMin.Y + F.Y;
	}

	/** Plaka, govde, kapi, yol karosu: SizeCells ve derze gore kurar, renkleri uygular. */
	void ApplyVisuals();

	/** Hayalet moduna gecer: collision yok, golge yok, yari saydam. */
	void MakeGhost(UMaterialInterface* GhostMaterial);
	void SetGhostColor(const FLinearColor& Color, float Opacity);

	/** Tasima: elde tutulurken gizli ve collision'siz; birakilinca geri gelir. */
	void SetHeld(bool bInHeld);

	/** Aile, platform yerelindeki StartLocal noktasindan kapiya yurur ve eve girer. */
	void StartResidentsArrival(const FVector& StartPlatformLocal);

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlateMID;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultCube;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DiscMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FxMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ResidentMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> EntranceMarks;

	/** Yol uzantilari: +X, +Y, -X, -Y (platform ekseni; yol hep yon 0'da durur). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> RoadLinks;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RoadLinkMIDs;

	/** Yolun sekli: viraj parcalari ya da cikmaz yarim karo + disk. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> RoadShape;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RoadShapeMIDs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> EntrancePaths;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> DecorParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WindowMIDs;

	float NightGlow = 0.f;
	float PopTime = -1.f;
	bool TickPop(float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ConeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FxRing;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FxRingMID;

	void BuildEntrance();
	void BuildDecor();
	void BuildHouseDecor();
	void BuildHospitalDecor();
	void BuildMallDecor();
	void BuildStopDecor();
	void BuildParkDecor();
	void AddWindowPart(const FVector& Loc, const FVector& Size);
	void ClearDecor();
	UStaticMeshComponent* AddDecor(UStaticMesh* InMesh, const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color, const FRotator& Rot = FRotator::ZeroRotator);
	void AddCypress(const FVector& Base, float Height);
	void AddWindowGrid(int32 Face, float Bottom, float Top, int32 Rows, int32 Cols, float WinW, float WinH, bool bSkipDoorColumn,
		bool bFlowers = false, bool bShutters = false);

	// Govde yuzleri: 0 +X (kapi), 1 +Y, 2 -X, 3 -Y. Along = yuz boyunca, Out = govde merkezinden disari.
	FVector FaceLoc(int32 Face, float Along, float Out, float Z) const;
	FVector FaceSize(int32 Face, float AlongSize, float OutSize, float H) const;
	float FaceDepth(int32 Face) const;   // yuzun govde merkezine uzakligi
	float FaceHalfLen(int32 Face) const; // yuzun yari boyu
	/** Cerceveli pencere: cam (gece yanar), krem cerceve, dikey kayit; istenirse cicek kutusu ve panjur. */
	void AddFramedWindow(int32 Face, float Along, float Z, float W, float H, bool bFlowers, bool bShutters);
	/** Cizgili, one egik tente; on kenarinda yuvarlak firfirlar. ZTop = duvara bitistigi yukseklik. */
	void AddAwning(int32 Face, float Along, float ZTop, float Width, float Depth, const FLinearColor& Color);
	void AddPlanter(const FVector& Center, const FVector& Size, int32 Flowers);
	void AddLamp(const FVector& Base, float Height);
	void AddRoundTree(const FVector& Base, float Height);
	/** Besik cati: govde boyunca 45 derece cevrilmis kup (alinlik) + iki cati levhasi. Mahya X boyunca. */
	void AddGableRoof(float Top, const FLinearColor& Gable, const FLinearColor& Roof);
	void BuildRoadLinks();
	void BuildRoadShape();
	UStaticMeshComponent* MakePart(const FLinearColor& Color, bool bForGhost, UStaticMesh* InMesh = nullptr);
	bool TickResidents(float DeltaSeconds);
	bool TickFx(float DeltaSeconds);
	void SetSurfaceTint(const FLinearColor& C);

	float PlateSizeX() const { return FMath::Max(1.f, SizeCells.X * GridSize - TileGap); }
	float PlateSizeY() const { return FMath::Max(1.f, SizeCells.Y * GridSize - TileGap); }
	float BodyHalfX() const { return 0.5f * PlateSizeX() - BodyInsetCm; }
	float BodyHalfY() const { return 0.5f * PlateSizeY() - BodyInsetCm; }

	TArray<FVector> ResidentPath;    // bina yerelinde duraklar, son durak kapi
	TArray<FVector> ResidentOffsets; // her figurun yol uzerindeki yan kaymasi
	TArray<int32> ResidentStep;
	uint8 RoadLinkMask = 0;
	uint8 RoadShapeMask = 0xFF; // son kurulan sekil (0xFF: hic kurulmadi)
	bool bEntranceConnected = false;
	bool bGhost = false;
	bool bResidentsStarted = false;

	static constexpr float FxNone = -1000.f;
	float FxTime = FxNone; // FxNone: efekt yok; <0: gecikmede; 0..ConnectFxSeconds: oynuyor
	bool bFxRing = true;
	FVector BodyBaseScale = FVector::OneVector;
};
