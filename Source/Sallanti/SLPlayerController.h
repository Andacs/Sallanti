// Girdi ve slot secimi. Enhanced Input asset'leri calisma aninda kurulur: kimse .uasset beklemesin.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SLPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class USLBuildComponent;

UCLASS()
class SALLANTI_API ASLPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASLPlayerController();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<USLBuildComponent> Build;

	// --- Olcum/test komutlari (konsol; Shipping'de konsol yok) ---

	/** Slot sec, hayaleti yerel (X,Y) cm'ye nisanla, yerlestir. */
	UFUNCTION(Exec)
	void SLPlace(int32 Slot, float LocalX, float LocalY);

	/** Slot sec ve hayaleti yerel (X,Y)'de tut (yerlestirmeden). */
	UFUNCTION(Exec)
	void SLGhost(int32 Slot, float LocalX, float LocalY);

	/** Platform durumunu ve kenarlarin dunya Z'sini loga yaz. */
	UFUNCTION(Exec)
	void SLReport();

	/** Imleci geri ver (SLGhost/SLPlace nisanini kapatir). */
	UFUNCTION(Exec)
	void SLMouse();

	/** Uygulama sirasi 1-6'yi sirayla calistirir; her adimda ekran goruntusu + SL_REPORT. Sonda cikar. */
	UFUNCTION(Exec)
	void SLSelfTest();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void OnPlace();
	void OnCancel();
	void OnRotate();
	void OnSlot1();
	void OnSlot2();
	void OnSlot3();
	void OnSlot4();

	void SelfTestStep();
	void MouseAtLocal(int32 Slot, const FVector& Local);
	void LogGhost(const TCHAR* Label);
	void Shot(const TCHAR* Name);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;

	FTimerHandle SelfTestTimer;
	int32 SelfTestIndex = 0;
};
