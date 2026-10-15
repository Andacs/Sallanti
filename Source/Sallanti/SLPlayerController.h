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
	/** Tahtayi cevir (Z / X, HUD oklari). */
	void RotateView(int32 Steps);

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

	/** Derlemeden ayar: platform, yerlestirme ya da kameradaki EditAnywhere bir degeri degistirir.
	 *  Ornek: SLSet PlatformMass 10  /  SLSet TiltSpeed 8  /  SLSet Distance 2600 */
	UFUNCTION(Exec)
	void SLSet(const FString& Name, const FString& Value);

	/** Degistirilebilir butun sayilari konsola listeler. */
	UFUNCTION(Exec)
	void SLKnobs();

	/** 1. bolumu bastan sona oynar: ev + aile, kilitli slot, yanlis AVM -> devriliyor, fareyle tasima ->
	 *  kurtarma, yollarla baglama -> bolum sonu. Her adimda ekran goruntusu + SL_REPORT. Sonda cikar. */
	UFUNCTION(Exec)
	void SLTestLevel1();

	/** 2. bolumu oynar (L_Test?Bolum=2 ile acilmis olmali): Kaya ailesi + hastane + AVM + yol, gecersiz ve
	 *  gecerli durak, otobus, Demir ailesi uzak eve -> mutsuz, evi duraga yakina tasi -> mutlu -> bolum sonu. */
	UFUNCTION(Exec)
	void SLTestLevel2();

	/** Trailer cekimi: 3. bolumu sinematik oynar, her kareyi REC_f_#####.png olarak kaydeder.
	 *  Sabit adim icin oyunu "-benchmark -fps=30" ile ac. Tools/trailer_yap.ps1 karelerden MP4 yapar. */
	UFUNCTION(Exec)
	void SLSinema();

	/** Ana menuyu gezer (ana sayfa, bolumler, nasil oynanir, jenerik), her sayfada ekran goruntusu. */
	UFUNCTION(Exec)
	void SLTestMenu();

	/** N. bolumu bastan ac (0 menu, 1-3). */
	UFUNCTION(Exec)
	void SLBolum(int32 Number);

	/** 3. bolumu oynar (L_Test?Bolum=3): uc ailenin istekleri, park, AVM'ye yakin ev -> mutsuz -> tasi. */
	UFUNCTION(Exec)
	void SLTestLevel3();

	/** Saati sabitle (0..24, gece-gunduz denemesi). Negatif = hamleye don. */
	UFUNCTION(Exec)
	void SLSaat(float Hour);

	/** Sag tikin "tiklama" sayilmasi icin en fazla surukleme, px. Fazlasi haritayi kaydirir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float PanClickThresholdPx = 6.f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void OnPlace();
	void OnPlaceReleased();
	void OnPanPressed();
	void OnPanReleased();
	void OnCenterCamera();
	void OnZoom(const struct FInputActionValue& Value);
	void OnToggleDrawer();
	void OnCancel();
	void OnRotate();
	void OnRotateBack();
	void OnSlot1();
	void OnSlot2();
	void OnSlot3();
	void OnSlot4();
	void OnSlot5();
	void OnSlot6();
	void OnRotateViewLeft();
	void OnRotateViewRight();
	void OnToggleMusic();
	void OnNextLevel();
	void OnUndo();
	void OnRestart();
	void OnAgain();
	void OnToggleHelp();

	/** SLSet/SLKnobs'un dolastigi nesneler: platform, BuildComponent, kamera pawn'i. */
	TArray<UObject*> GetKnobTargets() const;

	void TestLevel1Step();
	void TestLevel2Step();
	void TestLevel3Step();
	void CinemaStep();
	void MenuTestStep();
	bool bRecording = false;
	int32 RecFrame = 0;
	float CinemaTime = 0.f;
	float CaptionUntil = -1.f;
	void Caption(const TCHAR* Text, float Seconds);
	void MouseAtLocal(int32 Slot, const FVector& Local);
	void MoveMouseToLocal(const FVector& Local);
	void MoveMouseToScreen(const FVector2D& Screen);
	void LogGhost(const TCHAR* Label);
	void Shot(const TCHAR* Name);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;

	bool bPanning = false;
	float PanMoved = 0.f;
	FVector2D LastPanMouse = FVector2D::ZeroVector;

	FTimerHandle SelfTestTimer;
	int32 SelfTestIndex = 0;
};
