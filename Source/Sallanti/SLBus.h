// Otobus (2. bolum): duraga bagli yol aginda gidip gelir, durakta bekler. Kutlesi yok (dengeyi
// bozmaz), platforma bagli (platformla egilir), collision yok. Rotayi bolum yoneticisi verir.
// Govde motorun hazir sekilleriyle; Mert'in otobus mesh'i gelince BodyMesh'e baglanir.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLBus.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class SALLANTI_API ASLBus : public AActor
{
	GENERATED_BODY()

public:
	ASLBus();

	virtual void Tick(float DeltaSeconds) override;

	/** cm/sn, platform yerelinde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus", meta = (ClampMin = "5"))
	float Speed = 90.f;

	/** Durakta bekleme, sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus", meta = (ClampMin = "0"))
	float StopSeconds = 1.5f;

	/** Rotanin ucunda geri donmeden once bekleme, sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus", meta = (ClampMin = "0"))
	float TurnSeconds = 0.4f;

	/** Donus hizi, derece/sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus")
	float TurnRate = 360.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus")
	FLinearColor BodyColor = FLinearColor(1.f, 0.78f, 0.36f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus")
	FLinearColor StripeColor = FLinearColor(0.30f, 0.72f, 0.70f);

	/** Bos = koddan cizilen otobus. Pivot taban ortasi, on yuz +X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Otobus")
	TObjectPtr<UStaticMesh> BodyMesh;

	/** Rota: platform yerelinde hucre merkezleri (ya da ilk bos rota). Stops: rotadaki durak indeksleri.
	 *  Yeni rota eskisine en yakin noktadan devam eder; ayni rota tekrar verilirse bir sey olmaz. */
	void SetRoute(const TArray<FVector>& Points, const TArray<int32>& Stops);

	bool IsWaitingAtStop() const { return WaitLeft > 0.f && bWaitIsStop; }

	/** Rotadaki ilerleme (test/log): suanki nokta indeksi. */
	int32 GetRouteIndex() const { return Index; }

private:
	void BuildBody();
	UStaticMeshComponent* Part(UStaticMesh* M, const FVector& Loc, const FVector& Size, const FLinearColor& C, const FRotator& R = FRotator::ZeroRotator);

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cube;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cylinder;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Mat;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Body;

	TArray<FVector> Route;
	TSet<int32> StopSet;
	int32 Index = 0;  // su an bulundugu / ayrildigi nokta
	int32 Dir = 1;    // +1 ileri, -1 geri
	float Alpha = 0.f; // Index -> Index+Dir arasi 0..1
	float WaitLeft = 0.f;
	bool bWaitIsStop = false;
	float Yaw = 0.f;
	bool bBuilt = false;
};
