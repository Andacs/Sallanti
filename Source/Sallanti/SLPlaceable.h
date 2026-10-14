// Yerlestirilen nesnenin tabani. Pivot tabanin ortasinda (08_ASSET_SPEC 3. bolum).
// Hem gercek nesne hem hayalet olarak kullanilir; hayalet collision'siz ve yari saydamdir.
// Yerlestirilen nesne platforma bagli ve sabittir (fizik yok); platform devrilince onunla devrilir.

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Tur kimligi: gorevler buna bakar (Ev, Hastane, AVM, Yol). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FName TypeId;

	/** Dengeye katki agirligi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float Mass = 1.f;

	/** Izgara adimi, cm. Asset spec'iyle ayni: 50. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float GridSize = 50.f;

	/** Boyut, modul cinsinden (G x D x Y). 1 modul = GridSize cm. Duz nesnede Y yok sayilir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FIntVector SizeCells = FIntVector(1, 1, 1);

	/** Duz nesne (yol): yuksekligi FlatHeightCm, ustune bir sey konmaz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	bool bFlat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float FlatHeightCm = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FLinearColor Tint = FLinearColor(0.62f, 0.45f, 0.28f);

	/** Bos = olcekli gri kup. Modelcinin mesh'i verilirse olceklenmez; boyutu zaten SizeCells olmali. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	TObjectPtr<UStaticMesh> DisplayMesh;

	/** Bu binaya tasinacak kisi sayisi (Ev: aile). 0 = kimse gelmez. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	int32 Residents = 0;

	/** Aile uyesinin boyu, cm (08_ASSET_SPEC: insan 160). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float ResidentHeightCm = 160.f;

	/** Yurume hizi, cm/sn. 120'de kenara yakin eve yurume 0.8 sn surup gozden kaciyordu. */
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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bRotated = false;

	/** Oyuncunun elinde (tasiniyor): gizli, dengeye ve hucre doluluguna katilmaz. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bHeld = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bResidentsArrived = false;

	bool HasResidentsStarted() const { return bResidentsStarted; }

	/** Platform uzerindeki XY ayak izi, donmeyi hesaba katarak. */
	FIntPoint GetFootprint() const { return bRotated ? FIntPoint(SizeCells.Y, SizeCells.X) : FIntPoint(SizeCells.X, SizeCells.Y); }

	/** Ustune konabilecek yukseklik, modul cinsinden. Duz nesne 0. */
	int32 GetStackHeight() const { return bFlat ? 0 : SizeCells.Z; }

	bool CoversCell(const FIntPoint& Cell) const
	{
		const FIntPoint F = GetFootprint();
		return Cell.X >= CellMin.X && Cell.Y >= CellMin.Y && Cell.X < CellMin.X + F.X && Cell.Y < CellMin.Y + F.Y;
	}

	/** Mesh'i SizeCells'e gore olcekler, pivotu tabana alir, rengi uygular. */
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
	TObjectPtr<UStaticMesh> DefaultCube;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ResidentMeshes;

	TArray<FVector> ResidentTargets; // bina yerelinde
	bool bGhost = false;
	bool bResidentsStarted = false;
};
