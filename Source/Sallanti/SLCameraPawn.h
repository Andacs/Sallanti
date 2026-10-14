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
	 *  600 cm'lik platform 16:9 kadraja sigmiyor (dikeyde %153); 2200'de %87. Tahta 650 cm'e cikinca
	 *  ayni oran icin 2200 * 650/600 ~ 2400. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float Distance = 2400.f;

	/** Yakinlastirma siniri, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ZoomMin = 1100.f;

	/** Uzaklastirma siniri, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ZoomMax = 3600.f;

	/** Tekerlegin bir tiki: mevcut uzakligin bu orani (carpimsal: yakinda ince, uzakta kaba adim). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float ZoomStepFraction = 0.12f;

	/** Yumusatma: hedef uzakliga yaklasma hizi (1/sn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ZoomInterpSpeed = 10.f;

	/** Bakilan nokta, pawn'a gore. Kuleler yukari uzadigi icin biraz yukarida. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	FVector LookAtOffset = FVector(0.f, 0.f, 40.f);

	/** Pozlama duzeltmesi, EV. Otomatik pozlama karton tahtayi asiri aciyordu: yesil karo (0.30,0.55,0.14)
	 *  ekranda (221,238,193), kahverengi derz (225,203,166) olcululdu, renkler ayirt edilmiyordu.
	 *  Isigin son ayari Mert'te; bu yalnizca okunurluk icin baslangic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ExposureBias = -1.5f;

	/** Tekerlek: + yakinlastir, - uzaklastir (tik sayisi). */
	void Zoom(float Ticks);

	float GetTargetDistance() const { return TargetDistance; }
	void SetTargetDistance(float D) { TargetDistance = FMath::Clamp(D, ZoomMin, ZoomMax); }

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void ApplyCamera();

private:
	float TargetDistance = 2400.f;
};
