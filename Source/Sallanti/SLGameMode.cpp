#include "SLGameMode.h"

#include "SLCameraPawn.h"
#include "SLPlatform.h"
#include "SLBuildComponent.h"
#include "SLPlayerController.h"
#include "SLTestHUD.h"
#include "EngineUtils.h"

ASLGameMode::ASLGameMode()
{
	DefaultPawnClass = ASLCameraPawn::StaticClass();
	PlayerControllerClass = ASLPlayerController::StaticClass();
	HUDClass = ASLTestHUD::StaticClass(); // Yigit'in WBP_HUD'u gelene kadar gelistirici paneli
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

void ASLGameMode::HandleCollapsed()
{
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
