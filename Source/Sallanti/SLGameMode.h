// Oyun modu. Platform seviyede varsa onu kullanir, yoksa kendisi dogurur:
// boylece kod L_Ana'nin icerigine bagli degil, seviye sahibi (Yigit) platformu istedigi gibi koyabilir.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SLGameMode.generated.h"

class ASLLevelDirector;
class ASLPlatform;

UCLASS()
class SALLANTI_API ASLGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASLGameMode();

	/** Platform devrildi mi? */
	UFUNCTION(BlueprintPure, Category = "Sallanti")
	bool IsGameOver() const { return bGameOver; }

	/** Bolumun butun gorevleri bitti mi? */
	UFUNCTION(BlueprintPure, Category = "Sallanti")
	bool IsLevelComplete() const { return bLevelComplete; }

	/** Oyun bitti ya da bolum bitti: Space yeniden baslatir. */
	bool IsRoundOver() const { return bGameOver || bLevelComplete; }

	/** Seviyedeki bolum yoneticisi; yoksa 1. bolum varsayilanlariyla dogurulur. */
	ASLLevelDirector* GetDirector();
	const ASLLevelDirector* GetDirector() const { return Director; }

	/** Yonetici son gorev bitince cagirir. */
	void HandleLevelComplete();

	/** Seviyedeki platform; yoksa dunya merkezine dogurulur. */
	ASLPlatform* GetPlatform();
	const ASLPlatform* GetPlatform() const { return Platform; }

protected:
	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UFUNCTION()
	void HandleCollapsed();

	UPROPERTY(Transient)
	TObjectPtr<ASLPlatform> Platform;

	UPROPERTY(Transient)
	TObjectPtr<ASLLevelDirector> Director;

	bool bGameOver = false;
	bool bLevelComplete = false;
};
