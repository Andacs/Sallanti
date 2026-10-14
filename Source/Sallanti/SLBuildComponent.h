// Oyuncudaki yerlestirme: slot secimi, imlecten isin, hayalet, izgaraya yapisma, gecerlilik, yerlestirme.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SLBuildComponent.generated.h"

class ASLPlaceable;
class ASLPlatform;
class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class ESLBuildState : uint8
{
	Idle,     // slot secilmedi
	Placing,  // hayalet aktif, gecerli konumda
	Blocked   // hayalet aktif ama gecersiz konumda
};

USTRUCT(BlueprintType)
struct FSLSlot
{
	GENERATED_BODY()

	/** Test ekraninda gorunen ad. Oyuncuya gosterilmez (UI metinsiz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Label;

	/** Modul cinsinden G x D x Y. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FIntVector SizeCells = FIntVector(1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	float Mass = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FLinearColor Tint = FLinearColor(0.62f, 0.45f, 0.28f);

	/** Bos = gri kup. Mert'in mesh'i gelince buraya. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TObjectPtr<UStaticMesh> Mesh;
};

UCLASS(ClassGroup = (Sallanti), meta = (BlueprintSpawnableComponent))
class SALLANTI_API USLBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USLBuildComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 1-4 tuslari. Spec'teki sabit 4 nesne (envanter yok). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yerlestirme")
	TArray<FSLSlot> Slots;

	/** Acik karton uzerinde okunsun diye koyu ve doygun (0.2/0.9/0.3 zor seciliyordu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yerlestirme")
	FLinearColor ValidColor = FLinearColor(0.0f, 0.55f, 0.12f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yerlestirme", meta = (ClampMin = "0.05", ClampMax = "1"))
	float GhostOpacity = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yerlestirme")
	FLinearColor BlockedColor = FLinearColor(0.95f, 0.15f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yerlestirme")
	TObjectPtr<UMaterialInterface> GhostMaterial;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	ESLBuildState State = ESLBuildState::Idle;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 SelectedSlot = INDEX_NONE;

	/** Yerlestirilen nesne sayisi (kazanma kosulu buradan okuyacak). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 PlacedCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void SelectSlot(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void Cancel();

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void RotateGhost();

	/** Hayalet gecerliyse gercek nesneyi dogurur. Yerlesti mi? */
	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	bool TryPlace();

	/** Son yerlestirileni kaldirir (test icin). */
	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void UndoLast();

	/** Olcum/test: imlec yerine bu yerel XY'yi kullan. Kapatmak icin ClearAimOverride. */
	void SetAimOverride(const FVector2D& LocalXY);
	void ClearAimOverride() { bAimOverride = false; }

	FIntPoint GetGhostCellMin() const { return GhostCellMin; }
	int32 GetGhostLevel() const { return GhostLevel; }

private:
	ASLPlatform* GetPlatform() const;
	bool GetAimLocal(FVector2D& OutLocalXY) const;
	void UpdateGhost();
	void DestroyGhost();
	void ConfigurePlaceable(ASLPlaceable* P, const FSLSlot& Slot) const;

	UPROPERTY(Transient)
	TObjectPtr<ASLPlaceable> Ghost;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ASLPlaceable>> PlacedStack;

	FIntPoint GhostCellMin = FIntPoint::ZeroValue;
	int32 GhostLevel = 0;
	bool bGhostRotated = false;
	bool bAimOverride = false;
	FVector2D AimOverride = FVector2D::ZeroVector;
};
