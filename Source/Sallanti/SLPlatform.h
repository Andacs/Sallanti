// Sallanan platform. Agirlik merkezinin ve egimin TEK sahibi: HUD, ses, kaybetme kontrolu buradan okur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLPlatform.generated.h"

class ASLPlaceable;
class UStaticMeshComponent;

UCLASS()
class SALLANTI_API ASLPlatform : public AActor
{
	GENERATED_BODY()

public:
	ASLPlatform();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Yerlestirilen nesneler buna baglanir; egim bu bileseni dondurur. Ust yuzey yerel Z=0. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<USceneComponent> TiltPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UStaticMeshComponent> Deck;

	// --- Denge (09_MEKANIK_SPEC 7. bolum; hepsi baslangic tahmini) ---

	/** Agirlik merkezi ofseti -> egim, derece/cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge")
	float TiltPerCm = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float MaxTilt = 12.f;

	/** Platformun merkezdeki kendi kutlesi: CoM = Sum(m*r) / (PlatformMass + Sum(m)).
	 *  0 = spec formulu; ilk kutu tek basina tam egim verir. Buyudukce ilk nesneler hafif kalir, yuk birikir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (ClampMin = "0"))
	float PlatformMass = 0.f;

	/** Hedef egime yaklasma hizi, derece/sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge")
	float TiltSpeed = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float SwayAmplitude = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Seconds", ClampMin = "0.1"))
	float SwayPeriod = 4.f;

	/** Sallanmanin platform uzerindeki yonu (yerel XY'de, derece). 0 = +X kenari iner/kalkar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float SwayDirection = 0.f;

	/** Adim 7: bu egimi gecince nesneler fizige birakilir. Henuz kullanilmiyor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float ReleaseTilt = 8.f;

	/** Adim 8: kaybetme esigi. Henuz kullanilmiyor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float FailTilt = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Seconds"))
	float FailSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge")
	float FailZ = -200.f;

	// --- Boyut ---

	/** Platform kenari / 2, cm. 300 = 600x600 (08_ASSET_SPEC: 12x12 modul). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Platform")
	float HalfExtent = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Platform")
	float DeckThickness = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Platform")
	float GridSize = 50.f;

	/** Ekrana CoM/egim yazar ve CoM'u isaretler. Ayar yaparken acik kalsin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Debug")
	bool bDebugDraw = true;

	// --- Okunan durum (HUD/ses buradan okur) ---

	/** Yerel XY, cm. Nesne yoksa (0,0). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	FVector2D CenterOfMass = FVector2D::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	float TotalMass = 0.f;

	/** Sallanma haric egim vektoru: yon = inen taraf (yerel XY), boy = derece. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	FVector2D CurrentTilt = FVector2D::ZeroVector;

	/** Sallanma dahil toplam egim, derece. Kaybetme ve HUD bunu okur. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	float CurrentTiltDegrees = 0.f;

	int32 GetCellCount() const { return FMath::Max(1, FMath::RoundToInt(2.f * HalfExtent / GridSize)); }

	/** Hucre indeksinin (0..N) yerel koordinati. Hucre 0'in sol alt kosesi = -HalfExtent. */
	float CellToLocal(float Cell) const { return -HalfExtent + Cell * GridSize; }

	void RegisterPlaceable(ASLPlaceable* Placeable);
	const TArray<TWeakObjectPtr<ASLPlaceable>>& GetPlaceables() const { return Placeables; }

	/** Ayak izi [Min, Min+Size) hucrelerinin ustundeki en yuksek seviye (hucre cinsinden). */
	int32 GetStackLevel(FIntPoint CellMin, FIntPoint Footprint) const;

	/** Debug/olcum: yerel bir noktanin su anki dunya Z'si. */
	float GetWorldZOfLocal(const FVector& Local) const;

private:
	void RecomputeCenterOfMass();
	void ApplyDeckSize();

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ASLPlaceable>> Placeables;

	float SwayTime = 0.f;
};
