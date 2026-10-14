// Sabit izometrik kamera. Oyuncu dondurmez; fare tekerlegiyle yakinlastirir/uzaklastirir.
// Platform egilince kamera egilmez.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SLCameraPawn.generated.h"

class UCameraComponent;

UCLASS()
class SALLANTI_API ASLCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ASLCameraPawn();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (Units = "Degrees"))
	float FieldOfView = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (Units = "Degrees"))
	float Yaw = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (Units = "Degrees"))
	float Pitch = -35.f;

	/** Bakilan noktaya (platform merkezi) baslangic uzakligi, cm. Spec 1400 diyordu; olculdu: 1400'de
	 *  600 cm'lik platform 16:9 kadraja sigmiyor (dikeyde %153). 2200'de platform kadrajin %87'si. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float Distance = 2200.f;

	/** Yakinlastirma siniri, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ZoomMin = 1100.f;

	/** Uzaklastirma siniri, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ZoomMax = 3400.f;

	/** Tekerlegin bir tiki: mevcut uzakligin bu orani (carpimsal: yakinda ince, uzakta kaba adim). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float ZoomStepFraction = 0.12f;

	/** Yumusatma: hedef uzakliga yaklasma hizi (1/sn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ZoomInterpSpeed = 10.f;

	/** Bakilan nokta, pawn'a gore. Kuleler yukari uzadigi icin biraz yukarida. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	FVector LookAtOffset = FVector(0.f, 0.f, 40.f);

	/** Tekerlek: + yakinlastir, - uzaklastir (tik sayisi). */
	void Zoom(float Ticks);

	float GetTargetDistance() const { return TargetDistance; }
	void SetTargetDistance(float D) { TargetDistance = FMath::Clamp(D, ZoomMin, ZoomMax); }

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void ApplyCamera();

private:
	float TargetDistance = 2200.f;
};
