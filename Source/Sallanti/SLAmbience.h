// Cozy cevre (14 Eki, Andac'in referans gorseli): lavanta arka plan, asagida suzulen bulutlar, tahtanin
// iki yaninda havada duran pastel adaciklar (pencereli binalar, servi agaclari), sicak isik.
// Tamamen motorun hazir sekilleriyle: Mert/Asil'in asset'leri gelince yerlerini alir.
// Platforma bagli DEGIL (egilmez), collision yok (imlec isinini tutmaz). Seviyede yoksa GameMode dogurur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLAmbience.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class SALLANTI_API ASLAmbience : public AActor
{
	GENERATED_BODY()

public:
	ASLAmbience();

	virtual void BeginPlay() override;

	/** Arka plan rengi (isiksiz): kameranin gordugu "gokyuzu". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor BackdropColor = FLinearColor(0.62f, 0.50f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor CloudColor = FLinearColor(0.97f, 0.92f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre", meta = (ClampMin = "0", ClampMax = "60"))
	int32 CloudCount = 22;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor IslandColor = FLinearColor(0.97f, 0.86f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor WindowColor = FLinearColor(0.35f, 0.50f, 0.78f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor TreeColor = FLinearColor(0.22f, 0.42f, 0.40f);

	/** Seviyedeki gunes/gokyuzu isigini sicak pastel tona cek (yalnizca oyun sirasinda; seviye dosyasina
	 *  yazilmaz). Mert isigi kendisi ayarlayinca kapatilir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	bool bTintLevelLights = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor SunColor = FLinearColor(1.f, 0.90f, 0.84f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor SkyLightColor = FLinearColor(0.85f, 0.78f, 1.f);

	/** Lux. Golgeler yumusak olsun diye gunes kisik, gokyuzu isigi yuksek. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float SunIntensity = 5.f;

	/** Gunes yonu: kameranin gordugu iki yuz de (-X, -Y) aydinlansin, golgeler binalarin arkasina dussun.
	 *  Eskisi (yaw -30) kameraya bakan sol yuzu tamamen golgede birakiyordu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FRotator SunRotation = FRotator(-50.f, 55.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float SkyLightIntensity = 2.f;

	/** Isiksiz arka plan ve bulut rengi carpani: isikli yuzeylerle ayni pozlamada okunsun diye. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float UnlitBrightness = 2.6f;

	/** Adacik binalarinin pastel renkleri (sirayla). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	TArray<FLinearColor> IslandHouseColors = {
		FLinearColor(0.98f, 0.92f, 0.84f), FLinearColor(0.97f, 0.80f, 0.80f), FLinearColor(0.80f, 0.90f, 0.86f),
		FLinearColor(0.90f, 0.84f, 0.96f) };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor IslandRoofColor = FLinearColor(0.88f, 0.42f, 0.38f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	int32 RandomSeed = 1410;

	/** Adacik: bir kaide + 1-3 pastel bina + servi agaclari. Platform yerelinde, cm. */
	void BuildIsland(const FVector& Center, float Size, FRandomStream& Rng);

	/** Servi agaci: ince uzun koni + kisa govde. Dekor icin (adacik ve bina plakalari). */
	static void AddCypress(AActor* Owner, USceneComponent* Parent, UStaticMesh* Cone, UStaticMesh* Cube,
		UMaterialInterface* Mat, const FVector& Base, float Height, const FLinearColor& Color);

private:
	UStaticMeshComponent* AddShape(UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color, bool bUnlit, float Opacity = 1.f);
	void TintLights();

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cube;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Sphere;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cone;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> LitMat;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> UnlitMat;
};
