// Gelistirici test ekrani: secili slot, hucre, gecerlilik, egim cubugu, kontroller.
// Oyuncu HUD'u degil (o Yigit'in WBP_HUD'u); H ile kapanir. Metinde emoji yok.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SLTestHUD.generated.h"

UCLASS()
class SALLANTI_API ASLTestHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Test")
	bool bShowTestPanel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Test")
	float TextScale = 1.2f;

private:
	/** Uyari / devrilme mesaji. Test paneli gizliyken de cizilir (oynanis geri bildirimi). */
	void DrawCenterMessage(const class ASLPlatform& P);
	void Line(float X, float& Y, const FString& Text, const FLinearColor& Color = FLinearColor::White);
};
