// Bolum yoneticisi: gorev listesi, slot kilitleri, ailenin gelisi, yol baglantisi, bolum sonu.
// Gorevler veridir (EditAnywhere): yeni bolum icin kod degil, bu listenin baska bir kopyasi yeter.
// Seviyede bir tane varsa o kullanilir; yoksa GameMode 1. bolumun varsayilanlariyla dogurur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLLevelDirector.generated.h"

class ASLFamily;
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

	/** Ailenin konusma balonu: gorevi aile verir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString Speech;

	/** Gorev bitince ailenin kisa tesekkuru (bos = yok). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti")
	FString DoneSpeech;

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

	/** Bolum bitince ailenin son sozu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	FString FinalSpeech = TEXT("Mahallemiz hazır. Teşekkürler başkan!");

	/** Tesekkur balonunun ekranda kalma suresi, sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	float DoneSpeechSeconds = 2.5f;

	/** Ailenin bolum basinda bekledigi yer (platform yerelinde, cm). Varsayilan: on kenar, kameraya yakin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Bolum")
	FVector FamilyWaitLocal = FVector(-230.f, -250.f, 0.f);

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

	/** Su an gosterilecek konusma balonu. false = balon yok. */
	bool GetSpeech(FString& OutText, FVector& OutWorldAnchor) const;

private:
	ASLPlatform* GetPlatform() const;
	USLBuildComponent* GetBuild() const;
	void ApplyUnlocks();
	void StartFamilies();
	bool EvaluateTask(const FSLTask& Task, int32& OutProgress, int32& OutGoal) const;
	void CompleteLevel();

	void SpawnFamily();

	/** Kapisina yol degen binalar: yeni baglananda efekt (bina halkasi + yol boyunca akan parlama). */
	void UpdateConnectionFx();

	TSet<TWeakObjectPtr<ASLPlaceable>> ConnectedBuildings; // zayif isaretci: GC'ye UPROPERTY gerekmez

	UPROPERTY(Transient)
	TObjectPtr<ASLFamily> Family;

	/** Ailenin yasadigi ev (ilk ev). Sonraki gorevlerin balonu bunun ustunden. */
	TWeakObjectPtr<ASLPlaceable> FamilyHome;

	FString DoneText;
	FString LastSpeech;
	float DoneUntil = -1.f;

	int32 UnlockedForTask = -1;
};
