// Oyun ekrani: denge bari, gorevler, slot cubugu, uyari/bolum mesaji. H ile gelistirici test paneli.
// Yigit'in WBP_HUD'u gelince bu sinif yalnizca test paneline iner; okudugu degerler BlueprintReadOnly.
// Metinde emoji yok (04_UNREAL_TUZAKLARI: fontta olmayan glif oyunu donduruyor).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SLHUD.generated.h"

class ASLLevelDirector;
class ASLPlatform;
class USLBuildComponent;

UCLASS()
class SALLANTI_API ASLHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Gelistirici paneli (sayilar, CoM, ayarlar). Oyuncu ekrani her zaman cizilir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|HUD")
	bool bShowTestPanel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|HUD")
	float TextScale = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|HUD")
	float BalanceBarWidth = 520.f;

private:
	void DrawBalanceBar(const ASLPlatform& P);
	void DrawTasks(const ASLLevelDirector& D);
	void DrawSlotBar(const USLBuildComponent& B);
	void DrawCenterMessage(const ASLPlatform& P, bool bLevelComplete, const FString& LevelTitle);
	void DrawTestPanel(const ASLPlatform& P, const USLBuildComponent& B);

	void Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bLarge = false);
	void Line(float X, float& Y, const FString& S, const FLinearColor& Color = FLinearColor::White);
	FVector2D Measure(const FString& S, float Scale, bool bLarge);
};
