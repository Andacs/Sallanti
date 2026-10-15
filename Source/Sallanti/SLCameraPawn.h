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
	/** -35'te yol yaparken binalarin arkasi gorunmuyordu (Andac, 14 Eki). -45: daha yukaridan. */
	float Pitch = -45.f;

	/** Bakilan noktaya (platform merkezi) baslangic uzakligi, cm. Spec 1400 diyordu; olculdu: 1400'de
	 *  600 cm'lik platform 16:9 kadraja sigmiyor (dikeyde %153); 2200'de %87. Tahta 650 cm'e cikinca
	 *  ayni oran icin 2200 * 650/600 ~ 2400. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float Distance = 2900.f; // -45'te 650 cm tahta + 150 cm bina dikeyde kadrajin ~%90'i (hesaplandi)

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
	float ExposureBias = 0.f;

	/** Sabit pozlama (14 Eki, cozy): otomatik pozlama buyuk lavanta arka plana bakip sahneyi karartiyordu ve
	 *  zoom'la renk degisiyordu. Acikken pozlama FixedExposureEV100'de kilitli; pastel renkler her yakinlikta ayni. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	bool bFixedExposure = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float FixedExposureEV100 = 1.9f;

	/** Sag tikla surukleyince harita kayar. Kayma siniri, cm (bakis noktasi tahtanin bu kadar disina cikmaz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float PanLimit = 325.f;

	/** Surukleme hissi: 1 = imlecin altindaki nokta parmakla birlikte gider. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float PanSpeed = 1.f;

	/** Ekranda (DX, DY) piksel surukleme -> bakis noktasini kaydirir. ViewportW: ekran genisligi, px. */
	void PanByScreen(float DX, float DY, float ViewportW);
	void ResetPan() { PanOffset = FVector2D::ZeroVector; ApplyCamera(); }
	FVector2D GetPanOffset() const { return PanOffset; }

	/** Tekerlek: + yakinlastir, - uzaklastir (tik sayisi). */
	void Zoom(float Ticks);

	/** Tahtayi cevir (16 Eki): kamera tahtanin etrafinda 90 derece adimlarla doner (Z / X, HUD oklari). */
	void RotateView(int32 Steps);
	/** 0..3: bastaki bakistan kac ceyrek tur. Yeni binanin kapisi kameraya baksin diye BuildComponent okur. */
	int32 GetViewSteps() const { return ((ViewSteps % 4) + 4) % 4; }

	/** Menu ve trailer: kamera tahtanin etrafinda kendiliginden doner, derece/sn (0 = durur). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float AutoOrbitDegPerSec = 0.f;

	/** Kamerayi kendi sagina kaydirir, cm (eksi = tahta ekranda saga gider; menude sol tarafa yazi icin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float ViewShiftRight = 0.f;

	/** Donus yumusakligi (FInterpTo hizi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float RotateInterpSpeed = 7.f;

	float GetTargetDistance() const { return TargetDistance; }
	void SetTargetDistance(float D) { TargetDistance = FMath::Clamp(D, ZoomMin, ZoomMax); }

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void ApplyCamera();

private:
	float TargetDistance = 2900.f;
	float TargetYaw = 45.f;
	int32 ViewSteps = 0;
	FVector2D PanOffset = FVector2D::ZeroVector;
};
