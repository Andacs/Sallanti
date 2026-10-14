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

	/** Plaka kalinligi, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float PlateHeightCm = 2.f;

	/** Govdenin plaka kenarindan iceri payi, cm (her yandan). 5'ten 8'e: plaka-bina arasi daha genis (14 Eki). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float BodyInsetCm = 8.f;

	/** Kapidan yola uzanan giris yolu (kaldirim) rengi; yalnizca bina yola bagliyken gorunur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	FLinearColor EntrancePathColor = FLinearColor(0.86f, 0.82f, 0.72f);

	/** Viraj kac parcayla cizilir (cok = daha yuvarlak). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yol", meta = (ClampMin = "2", ClampMax = "16"))
	int32 CurveSegments = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FLinearColor Tint = FLinearColor(0.62f, 0.45f, 0.28f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FLinearColor PlateColor = FLinearColor(0.72f, 0.72f, 0.70f);

	/** Kapisi var mi? Kapi yerel +X yuzunde; yol ancak kapinin onundeki hucreye degerse baglanir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	bool bHasEntrance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	FLinearColor DoorColor = FLinearColor(0.18f, 0.11f, 0.07f);

	/** Kapi onu isareti (yerlestirirken gorunur). Yol ustune gelince ortulur: baglandigi gorulur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Giris")
	FLinearColor EntranceMarkColor = FLinearColor(0.95f, 0.75f, 0.15f);

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
	TObjectPtr<UStaticMeshComponent> FxRing;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FxRingMID;

	void BuildEntrance();
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
