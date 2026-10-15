// Aile: platformda duran, yuruyen, tiklanan karakterler (16 Eki, Andac: "kisilerin uzerine basip
// isteklerini ogrenecegiz", "karakter animasyonlari olsun"). Figur = govde (silindir) + bas (kure) + sac.
// Animasyon koddan: nefes (hafif buyuyup kuculme), mutluyken ara ara ziplama, mutsuzken yavas sallanma
// ve egik bas, yururken sekme, fare ustundeyken el sallama (bir figur ziplar), secilince altta halka.
// Kesme listesindeki "karakter animasyonu yok" maddesinin yerine Andac'in bu istegi gecti.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLFamily.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class SALLANTI_API ASLFamily : public AActor
{
	GENERATED_BODY()

public:
	ASLFamily();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	int32 Members = 3;

	/** Spec 160 diyordu; ev 50 cm'e inince oran icin gecici 35. Karar Asil'de. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float HeightCm = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	FLinearColor Tint = FLinearColor(0.15f, 0.35f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	FLinearColor SkinColor = FLinearColor(0.98f, 0.82f, 0.70f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float WalkSpeed = 60.f;

	/** Ziplama yuksekligi, cm (mutlu ya da tiklaninca). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float HopHeightCm = 7.f;

	/** Figurleri kurar (platform TiltPivot'una bagli dogduktan sonra). */
	void BuildFigures();

	/** Konusma balonunun baglanacagi dunya noktasi: figurlerin basinin ustu. */
	FVector GetSpeechAnchor() const;
	/** Tiklama icin govde ortasi (dunya). */
	FVector GetClickAnchor() const;

	void SetWaitingVisible(bool bVisible);
	bool IsWaitingVisible() const { return bWaitingVisible; }

	/** Ruh hali (animasyon): Happy01 mutluluk, bSelected altta halka + surekli ziplama, bHovered el sallama,
	 *  bAtHome eve yerlesti (kapinin onunde durur). */
	void SetMood(float Happy01, bool bSelected, bool bHovered);

	/** Platform yerelinde (TiltPivot) noktalari sirayla yuru. Son noktada FinalYaw'a doner. */
	void WalkPath(const TArray<FVector>& Points, float FinalYaw);
	bool IsWalking() const { return PathIndex < Path.Num(); }
	/** Yuruyus bitti (bir kez true doner, sonra sifirlanir). */
	bool ConsumeArrived() { const bool b = bArrivedFlag; bArrivedFlag = false; return b; }

	/** Tek seferlik ziplama (tiklaninca, mutlu olunca). */
	void Hop();

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Pivots;   // her figurun koku (ziplama/egilme)

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Heads;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Ring;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cube;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Sphere;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cylinder;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMat;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> UnlitMat;

	UStaticMeshComponent* Part(USceneComponent* Parent, UStaticMesh* M, const FVector& Loc, const FVector& Size, const FLinearColor& C);

	TArray<FVector> Path;
	int32 PathIndex = 0;
	float FinalYaw = 0.f;
	bool bArrivedFlag = false;
	TArray<FVector> Offsets;  // figurlerin kume ici yeri
	TArray<float> Phase;
	float Time = 0.f;
	float Happy = 0.5f;
	bool bSelected = false;
	bool bHovered = false;
	float HopTime = -10.f;     // tek seferlik ziplama baslangici
	float NextIdleHop = 2.f;
	bool bWaitingVisible = true;
};
