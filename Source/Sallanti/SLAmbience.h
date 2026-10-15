// Cozy cevre: lavanta arka plan, uc katmanda suzulen bulutlar (uzak yavas, yakin hizli), havada sallanan
// bahce adaciklari (binasiz: agac, cicek), suzulen sicak hava balonlari, tahtanin cevresinde donen kus
// suruleri, gece yildizlar. Gece-gunduz: bolum yoneticisinin saatine (her hamle ilerler) gore gunes,
// gokyuzu isigi, arka plan ve bulut rengi, pencerelerin yanmasi.
// Tamamen motorun hazir sekilleriyle: Mert/Asil'in asset'leri gelince yerlerini alir.
// Platforma bagli DEGIL (egilmez), collision yok. Seviyede yoksa GameMode dogurur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLAmbience.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
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
	virtual void Tick(float DeltaSeconds) override;

	// --- Renkler (gunduz / gece) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor BackdropColor = FLinearColor(0.62f, 0.50f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor BackdropNightColor = FLinearColor(0.12f, 0.12f, 0.34f);

	/** Gun dogumu / batimi tonu (gece ile gunduz arasinda en guclu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor DuskColor = FLinearColor(0.98f, 0.58f, 0.62f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor CloudColor = FLinearColor(0.97f, 0.92f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor CloudNightColor = FLinearColor(0.32f, 0.34f, 0.62f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre", meta = (ClampMin = "0", ClampMax = "60"))
	int32 CloudCount = 24;

	/** Ruzgar yonu (dunya XY) ve uc katmanin hizi, cm/sn: uzak, orta, yakin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FVector2D WindDir = FVector2D(1.f, -0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FVector LayerSpeeds = FVector(10.f, 22.f, 40.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor IslandColor = FLinearColor(0.97f, 0.86f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor GrassColor = FLinearColor(0.55f, 0.82f, 0.58f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor TreeColor = FLinearColor(0.22f, 0.48f, 0.42f);

	/** Balon renkleri (sirayla). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	TArray<FLinearColor> BalloonColors = {
		FLinearColor(0.98f, 0.62f, 0.62f), FLinearColor(1.f, 0.85f, 0.45f), FLinearColor(0.55f, 0.82f, 0.85f),
		FLinearColor(0.80f, 0.68f, 0.98f) };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre", meta = (ClampMin = "0", ClampMax = "12"))
	int32 BalloonCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre", meta = (ClampMin = "0", ClampMax = "6"))
	int32 FlockCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre", meta = (ClampMin = "0", ClampMax = "400"))
	int32 StarCount = 160;

	// --- Isik ---

	/** Seviyedeki gunes/gokyuzu isigini yonet (yalnizca oyun sirasinda; seviye dosyasina yazilmaz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	bool bTintLevelLights = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor SunColor = FLinearColor(1.f, 0.90f, 0.84f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor SunsetColor = FLinearColor(1.f, 0.62f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor MoonColor = FLinearColor(0.55f, 0.64f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor SkyLightColor = FLinearColor(0.85f, 0.78f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FLinearColor SkyLightNightColor = FLinearColor(0.40f, 0.45f, 0.95f);

	/** Lux. Golgeler yumusak olsun diye gunes kisik, gokyuzu isigi yuksek. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float SunIntensity = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float MoonIntensity = 2.2f;

	/** Ogle gunesi yonu: kameranin gordugu iki yuz de (-X, -Y) aydinlansin. Sabah/aksam alcalir, doner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	FRotator SunRotation = FRotator(-50.f, 55.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float SkyLightIntensity = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float SkyLightNightIntensity = 1.3f;

	/** Isiksiz arka plan ve bulut rengi carpani: isikli yuzeylerle ayni pozlamada okunsun diye. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	float UnlitBrightness = 2.6f;

	/** Saat degisince gosterilen saatin yaklasma hizi, saat/sn (hamleden sonra gunes kayar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zaman", meta = (ClampMin = "0.1"))
	float TimeLerpHoursPerSecond = 3.f;

	/** Test/ayar: >= 0 ise saati bu saate sabitler (0..24). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Zaman")
	float DebugHourOverride = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cevre")
	int32 RandomSeed = 1410;

	/** 0 gece .. 1 tam gunduz, saate gore (05-08 safak, 17-20 aksam). */
	static float DaylightAt(float Hour);

	/** Su an gosterilen saat (0..24) ve gunduz orani: HUD saat karti okur. */
	float GetShownHour() const { return FMath::Fmod(ShownHours, 24.f); }
	float GetDaylight() const { return Daylight; }

private:
	UStaticMeshComponent* AddShape(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color, bool bUnlit, float Opacity = 1.f, const FRotator& Rot = FRotator::ZeroRotator);
	USceneComponent* AddGroup(const FVector& Loc);
	void AddCypress(USceneComponent* Parent, const FVector& Base, float Height, const FLinearColor& Color);
	void BuildGardenIsland(const FVector& Center, float Size, FRandomStream& Rng);
	void BuildBalloon(const FVector& Center, float Size, const FLinearColor& Color, FRandomStream& Rng);
	void BuildFlock(int32 Index, FRandomStream& Rng);
	void ApplyTimeOfDay(bool bForce);
	float TargetHours() const;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cube;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Sphere;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cone;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cylinder;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> LitMat;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> UnlitMat;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BackdropMID;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Stars;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StarMID;

	/** Hareketli parcalar: grup + taban konumu + hareket bilgisi. */
	struct FDrifter
	{
		TWeakObjectPtr<USceneComponent> Comp;
		FVector Base = FVector::ZeroVector;
		float Speed = 0.f;     // ruzgar yonunde cm/sn (0 = yalnizca sallanir)
		float BobAmp = 0.f;    // dikey sallanma, cm
		float BobHz = 0.f;
		float Phase = 0.f;
		float Travel = 0.f;    // ruzgar yonunde kat edilen
	};
	TArray<FDrifter> Drifters;

	struct FCloudPuff
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> MID;
		float Opacity = 1.f;
	};
	TArray<FCloudPuff> Puffs;

	struct FBird
	{
		TWeakObjectPtr<USceneComponent> Comp;
		TWeakObjectPtr<USceneComponent> WingL;
		TWeakObjectPtr<USceneComponent> WingR;
		FVector Offset = FVector::ZeroVector; // suru icindeki yeri
		int32 Flock = 0;
		float FlapPhase = 0.f;
	};
	TArray<FBird> Birds;
	TArray<FVector> FlockParams; // X yaricap, Y aci hizi (rad/sn), Z yukseklik

	float Time = 0.f;
	float ShownHours = 7.f;
	float Daylight = 1.f;
	float LastAppliedHours = -100.f;
	bool bTimeInit = false;
};
