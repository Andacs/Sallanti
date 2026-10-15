// Oyun modu. Platform seviyede varsa onu kullanir, yoksa kendisi dogurur:
// boylece kod L_Ana'nin icerigine bagli degil, seviye sahibi (Yigit) platformu istedigi gibi koyabilir.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SLGameMode.generated.h"

class ASLLevelDirector;
class ASLPlatform;
class USoundBase;
class UAudioComponent;

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

	/** Oynanan bolum. Seviye URL'sinden okunur: "L_Test?Bolum=2". */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Bolum")
	int32 LevelNumber = 1;

	/** Ayni haritayi N. bolumle bastan acar (F5, Space, Enter hepsi buradan). */
	void TravelToLevel(int32 Number);

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** Hamle siniri doldu (3. bolum): oyun biter, devrilme gibi. */
	void HandleOutOfMoves();
	bool IsOutOfMoves() const { return bOutOfMoves; }

	// --- Muzik ---

	/** Cozy arka plan muzigi (Tools/muzik_uret.py yer tutucusu). Dongulu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Ses")
	TObjectPtr<USoundBase> Music;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Ses", meta = (ClampMin = "0", ClampMax = "1"))
	float MusicVolume = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Ses", meta = (ClampMin = "0"))
	float MusicFadeInSeconds = 2.f;

	/** M tusu ve HUD'daki hoparlor dugmesi. Bolum degisince sifirlanir (bilerek: her bolum muzikle acilir). */
	void ToggleMusic();
	bool IsMusicOn() const { return bMusicOn; }

	/** Ses efekti iste. Ayni karede birden fazla istek gelirse yalnizca en yuksek oncelikli calar
	 *  ("bir olay, bir ses": yol konup ayni anda baglaninca tik degil can duyulur). */
	static void PlaySfx(const UObject* WorldContext, FName Name, int32 Priority = 0, float Volume = 1.f, float Pitch = 1.f);

	/** Ses bankasi: ad -> ses. Tasarimci sesleri ayni adla /Game/Sallanti/Audio'ya gelir (Tools/sesleri_import.py). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Ses")
	TMap<FName, TObjectPtr<USoundBase>> Sounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Ses", meta = (ClampMin = "0", ClampMax = "2"))
	float MasterSfxVolume = 1.f;

	virtual void Tick(float DeltaSeconds) override;

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
	bool bOutOfMoves = false;
	bool bMusicOn = true;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComp;

	struct FQueuedSfx { FName Name; int32 Priority = -1; float Volume = 1.f; float Pitch = 1.f; };
	FQueuedSfx QueuedSfx;

	/** Onemli bir ses caldiktan sonra kisa sure daha onemsizleri sustur: gorev sesi + bir kare sonra
	 *  konusma miriltisi iki ses gibi duyuluyordu (log). */
	int32 ShadowPriority = -1;
	float ShadowUntil = -1.f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Ses")
	float SfxShadowSeconds = 0.4f;
};
