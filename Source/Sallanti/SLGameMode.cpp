#include "SLGameMode.h"

#include "SLCameraPawn.h"
#include "SLLevelDirector.h"
#include "SLPlatform.h"
#include "SLBuildComponent.h"
#include "SLPlayerController.h"
#include "SLHUD.h"
#include "EngineUtils.h"

ASLGameMode::ASLGameMode()
{
	DefaultPawnClass = ASLCameraPawn::StaticClass();
	PlayerControllerClass = ASLPlayerController::StaticClass();
	HUDClass = ASLHUD::StaticClass(); // Yigit'in WBP_HUD'u gelene kadar oyun ekrani
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

void ASLGameMode::StartPlay()
{
	Super::StartPlay();
	GetDirector();
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
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Director = GetWorld()->SpawnActor<ASLLevelDirector>(ASLLevelDirector::StaticClass(), FTransform::Identity, Params);
	return Director;
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
