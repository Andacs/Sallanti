// Oyuncudaki yerlestirme: slot secimi, imlecten isin, hayalet, izgaraya yapisma, gecerlilik, yerlestirme,
// ve tasima (replace): yerlesik bir nesneyi kaldirip baska yere koymak.

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
	Idle,     // slot secilmedi, elde bir sey yok
	Placing,  // hayalet aktif, gecerli konumda
	Blocked   // hayalet aktif ama gecersiz konumda
};

USTRUCT(BlueprintType)
struct FSLSlot
{
	GENERATED_BODY()

	/** Gorevlerin baktigi tur kimligi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FName Id;

	/** Ekranda gorunen ad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Label;

	/** Modul cinsinden G x D x Y. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FIntVector SizeCells = FIntVector(1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	float Mass = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FLinearColor Tint = FLinearColor(0.62f, 0.45f, 0.28f);

	/** Duz nesne (yol): ince, ustune bir sey konmaz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	bool bFlat = false;

	/** Yerlesince gelen kisi sayisi (Ev: aile). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	int32 Residents = 0;

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

	/** 1-4 tuslari. Bolumun nesne katalogu (envanter yok). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Yerlestirme")
	TArray<FSLSlot> Slots;

	/** Slot acik mi? Bolum yoneticisi gorevlere gore acar. Bos dizi = hepsi acik. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	TArray<bool> SlotUnlocked;

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

	/** false = yerlestirme kapali (platform devrildi ya da bolum bitti). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bBuildEnabled = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 SelectedSlot = INDEX_NONE;

	/** Yerlestirilen nesne sayisi. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 PlacedCount = 0;

	/** Oyun bitti: hayaleti kaldirir (tasinan nesneyi yerine koyar), yerlestirmeyi kilitler. */
	void DisableBuilding();

	bool IsSlotUnlocked(int32 Index) const { return !SlotUnlocked.IsValidIndex(Index) || SlotUnlocked[Index]; }
	void SetSlotUnlocked(int32 Index, bool bUnlocked);
	int32 FindSlotById(FName Id) const { return Slots.IndexOfByPredicate([Id](const FSLSlot& S) { return S.Id == Id; }); }

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void SelectSlot(int32 Index);

	/** Hayaleti birakir. Tasima yapiliyorsa nesne eski yerine doner. */
	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void Cancel();

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void RotateGhost();

	/** Hayalet gecerliyse yerlestirir (ya da tasinan nesneyi yeni yerine koyar). */
	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	bool TryPlace();

	/** Tasima: imlecin altindaki yerlesik nesneyi kaldirir. Elde tutulurken agirligi dengeden cikar. */
	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	bool TryPickUpUnderCursor();

	/** Tasima: verilen nesneyi kaldirir (test ve UI icin). */
	bool PickUp(ASLPlaceable* Target);

	bool IsMoving() const { return Moving != nullptr; }
	const ASLPlaceable* GetMoving() const { return Moving; }

	/** Son yerlestirileni kaldirir (test icin). */
	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void UndoLast();

	/** Olcum/test: imlec yerine bu yerel XY'yi kullan. Kapatmak icin ClearAimOverride. */
	void SetAimOverride(const FVector2D& LocalXY);
	void ClearAimOverride() { bAimOverride = false; }

	FIntPoint GetGhostCellMin() const { return GhostCellMin; }
	int32 GetGhostLevel() const { return GhostLevel; }

	/** Gecersizligin sebebi (test paneli). */
	const TCHAR* GetBlockedReason() const { return BlockedReason; }

private:
	ASLPlatform* GetPlatform() const;
	bool GetAimLocal(FVector2D& OutLocalXY) const;
	void UpdateGhost();
	void DestroyGhost();
	void SpawnGhostFor(const FSLSlot& Slot);
	void ConfigurePlaceable(ASLPlaceable* P, const FSLSlot& Slot) const;
	void PutAt(ASLPlaceable* P, FIntPoint Cell, int32 Level, bool bRot) const;

	UPROPERTY(Transient)
	TObjectPtr<ASLPlaceable> Ghost;

	/** Tasinan nesne (elde). */
	UPROPERTY(Transient)
	TObjectPtr<ASLPlaceable> Moving;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ASLPlaceable>> PlacedStack;

	FIntPoint MovingOrigCell = FIntPoint::ZeroValue;
	int32 MovingOrigLevel = 0;
	bool bMovingOrigRotated = false;

	FIntPoint GhostCellMin = FIntPoint::ZeroValue;
	int32 GhostLevel = 0;
	bool bGhostRotated = false;
	bool bAimOverride = false;
	FVector2D AimOverride = FVector2D::ZeroVector;
	const TCHAR* BlockedReason = TEXT("");
};
