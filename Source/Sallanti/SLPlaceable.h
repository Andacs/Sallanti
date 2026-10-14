// Yerlestirilen nesnenin tabani. Pivot tabanin ortasinda (08_ASSET_SPEC 3. bolum).
// Hem gercek nesne hem hayalet olarak kullanilir; hayalet collision'siz ve yari saydamdir.

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Dengeye katki agirligi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float Mass = 1.f;

	/** Izgara adimi, cm. Asset spec'iyle ayni: 50. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	float GridSize = 50.f;

	/** Boyut, modul cinsinden (G x D x Y). 1 modul = GridSize cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FIntVector SizeCells = FIntVector(1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	FLinearColor Tint = FLinearColor(0.62f, 0.45f, 0.28f);

	/** Bos = olcekli gri kup. Modelcinin mesh'i verilirse olceklenmez; boyutu zaten SizeCells olmali. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Nesne")
	TObjectPtr<UStaticMesh> DisplayMesh;

	// --- Platform uzerindeki yeri (BuildComponent yazar, platform okur) ---

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	FIntPoint CellMin = FIntPoint::ZeroValue;

	/** Tabanin bulundugu yukseklik, modul cinsinden. 0 = platform yuzeyi. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 Level = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bRotated = false;

	/** Platform uzerindeki XY ayak izi, donmeyi hesaba katarak. */
	FIntPoint GetFootprint() const { return bRotated ? FIntPoint(SizeCells.Y, SizeCells.X) : FIntPoint(SizeCells.X, SizeCells.Y); }

	/** Mesh'i SizeCells'e gore olcekler, pivotu tabana alir, rengi uygular. */
	void ApplyVisuals();

	/** Hayalet moduna gecer: collision yok, golge yok, yari saydam. */
	void MakeGhost(UMaterialInterface* GhostMaterial);
	void SetGhostColor(const FLinearColor& Color, float Opacity);

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultCube;

	bool bGhost = false;
};
