// Oyun modu. Platform seviyede varsa onu kullanir, yoksa kendisi dogurur:
// boylece kod L_Ana'nin icerigine bagli degil, seviye sahibi (Yigit) platformu istedigi gibi koyabilir.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SLGameMode.generated.h"

class ASLPlatform;

UCLASS()
class SALLANTI_API ASLGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASLGameMode();

	/** Seviyedeki platform; yoksa dunya merkezine dogurulur. */
	ASLPlatform* GetPlatform();
	const ASLPlatform* GetPlatform() const { return Platform; }

	/** Adim 9: kazanmak icin gereken nesne. Henuz kullanilmiyor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Tur")
	int32 TargetObjectCount = 12;

	/** Adim 9: tur ust siniri. Henuz kullanilmiyor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Tur", meta = (Units = "Seconds"))
	float RoundSeconds = 120.f;

protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<ASLPlatform> Platform;
};
