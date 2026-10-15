#include "SLGameMode.h"

#include "SLAmbience.h"
#include "SLCameraPawn.h"
#include "SLLevelDirector.h"
#include "SLPlatform.h"
#include "SLBuildComponent.h"
#include "SLPlayerController.h"
#include "SLHUD.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogSLSfx, Log, All);

ASLGameMode::ASLGameMode()
{
	DefaultPawnClass = ASLCameraPawn::StaticClass();
	PlayerControllerClass = ASLPlayerController::StaticClass();
	HUDClass = ASLHUD::StaticClass(); // Yigit'in WBP_HUD'u gelene kadar oyun ekrani

	// Sert referans: paketlemede sesler cook'a girsin (yalnizca adla yuklenen asset cook'tan kacabilir).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // kare sonunda: o karenin butun istekleri toplandi
#define SL_SFX(N) { static ConstructorHelpers::FObjectFinder<USoundBase> F(TEXT("/Game/Sallanti/Audio/" #N "." #N)); if (F.Succeeded()) { Sounds.Add(TEXT(#N), F.Object); } }
	SL_SFX(SFX_Yerlestir) SL_SFX(SFX_YolKoy) SL_SFX(SFX_Kaldir) SL_SFX(SFX_Gecersiz) SL_SFX(SFX_Baglanti)
	SL_SFX(SFX_Gorev) SL_SFX(SFX_Bolum) SL_SFX(SFX_Gicirti_Dongu) SL_SFX(SFX_Devrilme) SL_SFX(SFX_Konusma)
	SL_SFX(SFX_Cekmece) SL_SFX(SFX_UI_Tik)
#undef SL_SFX
	static ConstructorHelpers::FObjectFinder<USoundBase> MusicFinder(TEXT("/Game/Sallanti/Audio/MUS_Cozy.MUS_Cozy"));
	if (MusicFinder.Succeeded())
	{
		Music = MusicFinder.Object;
	}
}

ASLPlatform* ASLGameMode::GetPlatform()
{
	if (Platform)
	{
		return Platform;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ASLPlatform> It(World); It; ++It)
	{
		Platform = *It;
		break;
	}
	if (!Platform)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Platform = World->SpawnActor<ASLPlatform>(ASLPlatform::StaticClass(), FTransform::Identity, Params);
	}
	if (Platform)
	{
		Platform->OnCollapsed.AddUniqueDynamic(this, &ASLGameMode::HandleCollapsed);
	}
	return Platform;
}

void ASLGameMode::PlaySfx(const UObject* WorldContext, FName Name, int32 Priority, float Volume, float Pitch)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	ASLGameMode* GM = World ? World->GetAuthGameMode<ASLGameMode>() : nullptr;
	if (GM && Priority < GM->ShadowPriority && World->GetRealTimeSeconds() < GM->ShadowUntil)
	{
		return; // az once daha onemli bir ses caldi
	}
	if (GM && Priority > GM->QueuedSfx.Priority)
	{
		GM->QueuedSfx = { Name, Priority, Volume, Pitch };
	}
}

void ASLGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (QueuedSfx.Priority < 0)
	{
		return;
	}
	if (const TObjectPtr<USoundBase>* S = Sounds.Find(QueuedSfx.Name); S && *S)
	{
		UGameplayStatics::PlaySound2D(this, *S, QueuedSfx.Volume * MasterSfxVolume, QueuedSfx.Pitch);
		ShadowPriority = QueuedSfx.Priority;
		ShadowUntil = GetWorld()->GetRealTimeSeconds() + SfxShadowSeconds;
		UE_LOG(LogSLSfx, Log, TEXT("SL_SFX %s (oncelik %d)"), *QueuedSfx.Name.ToString(), QueuedSfx.Priority);
	}
	else
	{
		UE_LOG(LogSLSfx, Warning, TEXT("SL_SFX yok: %s"), *QueuedSfx.Name.ToString());
	}
	QueuedSfx = FQueuedSfx();
}

void ASLGameMode::StartPlay()
{
	Super::StartPlay();
	GetDirector();
	if (Music)
	{
		MusicComp = UGameplayStatics::CreateSound2D(this, Music, MusicVolume, 1.f, 0.f, nullptr, true, false);
		if (MusicComp)
		{
			MusicComp->FadeIn(MusicFadeInSeconds, MusicVolume);
		}
	}
	// Cozy cevre: seviyeye konmadiysa platformun yerine dogur (L_Ana'ya Yigit koyarsa o kullanilir).
	if (!TActorIterator<ASLAmbience>(GetWorld()))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FTransform At = GetPlatform() ? FTransform(GetPlatform()->GetActorLocation()) : FTransform::Identity;
		GetWorld()->SpawnActor<ASLAmbience>(ASLAmbience::StaticClass(), At, Params);
	}
}

ASLLevelDirector* ASLGameMode::GetDirector()
{
	if (Director || !GetWorld())
	{
		return Director;
	}
	for (TActorIterator<ASLLevelDirector> It(GetWorld()); It; ++It)
	{
		Director = *It;
		return Director;
	}
	// Ertelenmis dogum: bolum verisi BeginPlay'den once yazilsin.
	Director = GetWorld()->SpawnActorDeferred<ASLLevelDirector>(ASLLevelDirector::StaticClass(), FTransform::Identity,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Director)
	{
		Director->ConfigureLevel(LevelNumber);
		Director->FinishSpawning(FTransform::Identity);
	}
	return Director;
}

void ASLGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	LevelNumber = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Bolum"), 0), 0, 3);
	bCreditsOnStart = UGameplayStatics::GetIntOption(Options, TEXT("Jenerik"), 0) != 0;
	UE_LOG(LogTemp, Log, TEXT("SL_BOLUM %d (secenekler: %s)"), LevelNumber, *Options);
}

void ASLGameMode::TravelToLevel(int32 Number, bool bCredits)
{
	const FString Map = UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName());
	UE_LOG(LogTemp, Log, TEXT("SL_TRAVEL %s bolum %d"), *Map, Number);
	UGameplayStatics::OpenLevel(this, FName(*Map), true, FString::Printf(TEXT("Bolum=%d%s"), Number, bCredits ? TEXT("?Jenerik=1") : TEXT("")));
}

void ASLGameMode::HandleLevelComplete()
{
	if (bGameOver)
	{
		return;
	}
	bLevelComplete = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASLPlayerController* PC = Cast<ASLPlayerController>(It->Get()))
		{
			PC->Build->DisableBuilding();
		}
	}
	UE_LOG(LogTemp, Log, TEXT("SL_LEVEL_DONE bolum tamamlandi"));
}

void ASLGameMode::ToggleMusic()
{
	bMusicOn = !bMusicOn;
	if (MusicComp)
	{
		MusicComp->AdjustVolume(0.5f, bMusicOn ? MusicVolume : 0.f);
	}
	UE_LOG(LogTemp, Log, TEXT("SL_MUSIC %s"), bMusicOn ? TEXT("acik") : TEXT("kapali"));
}

void ASLGameMode::HandleOutOfMoves()
{
	if (bLevelComplete || bGameOver)
	{
		return;
	}
	bGameOver = true;
	bOutOfMoves = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASLPlayerController* PC = Cast<ASLPlayerController>(It->Get()))
		{
			PC->Build->DisableBuilding();
		}
	}
	PlaySfx(this, TEXT("SFX_Devrilme"), 6, 0.6f, 0.8f);
	UE_LOG(LogTemp, Log, TEXT("SL_GAMEOVER hamle bitti"));
}

void ASLGameMode::HandleCollapsed()
{
	if (bLevelComplete)
	{
		return;
	}
	bGameOver = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASLPlayerController* PC = Cast<ASLPlayerController>(It->Get()))
		{
			PC->Build->DisableBuilding();
		}
	}
	UE_LOG(LogTemp, Log, TEXT("SL_GAMEOVER platform devrildi"));
}

AActor* ASLGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Kamera pawn'i platformun merkezine dogar ve oraya bakar; PlayerStart gerekmiyor.
	return GetPlatform();
}
