// Bolum yoneticisi: gorev listesi, slot kilitleri, ailenin gelisi, yol baglantisi, bolum sonu.
// Gorevler veridir (EditAnywhere): yeni bolum icin kod degil, bu listenin baska bir kopyasi yeter.
// Seviyede bir tane varsa o kullanilir; yoksa GameMode 1. bolumun varsayilanlariyla dogurur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLLevelDirector.generated.h"

class ASLPlaceable;
class ASLPlatform;
class USLBuildComponent;

UENUM(BlueprintType)
enum class ESLTaskKind : uint8
{
	PlaceType,     // TypeId'den Count tane yerlestir
	FamilyMovedIn, // TypeId binasina aile yerlessin
	ConnectRoads   // ConnectTypes'in her birinden biri ayni yol agina degsin
};

USTRUCT(BlueprintType)
struct FSLTask
{
	GENERATED_BODY()

	/** Ekranda gorunen gorev cumlesi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Text;

	/** Kucuk ipucu satiri (bos olabilir). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Hint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	ESLTaskKind Kind = ESLTaskKind::PlaceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FName TypeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	int32 Count = 1;

	/** ConnectRoads: birbirine baglanacak turler. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TArray<FName> ConnectTypes;

	/** Bu gorev BASLAYINCA acilan slotlar (tur kimligiyle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	TArray<FName> Unlocks;
};

UCLASS()
class SALLANTI_API ASLLevelDirector : public AActor
{
	GENERATED_BODY()

public:
	ASLLevelDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	FString LevelTitle = TEXT("1. Bölüm: İlk mahalle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	TArray<FSLTask> Tasks;

	/** Bolum sonu icin platformun dengede olmasi gerekir mi? (devriliyor durumunda bitmez) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	bool bRequireBalanceToComplete = true;

	// --- Durum (HUD okur) ---

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 CurrentTask = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	bool bLevelComplete = false;

	/** Mevcut gorevin ilerlemesi, ornegin yol gorevinde bagli bina sayisi. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 Progress = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	int32 ProgressGoal = 1;

	bool IsTaskDone(int32 Index) const { return Index < CurrentTask || bLevelComplete; }

	/** Yol agina degen hedef turu sayisi (HUD ve test icin). */
	int32 CountConnected(const TArray<FName>& Types) const;

private:
	ASLPlatform* GetPlatform() const;
	USLBuildComponent* GetBuild() const;
	void ApplyUnlocks();
	void StartFamilies();
	bool EvaluateTask(const FSLTask& Task, int32& OutProgress, int32& OutGoal) const;
	void CompleteLevel();

	int32 UnlockedForTask = -1;
};
