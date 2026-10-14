#include "SLPlayerController.h"

#include "SLBuildComponent.h"
#include "SLGameMode.h"
#include "SLPlatform.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "InputAction.h"
#include "InputMappingContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogSLInput, Log, All);

ASLPlayerController::ASLPlayerController()
{
	Build = CreateDefaultSubobject<USLBuildComponent>(TEXT("Build"));
	bShowMouseCursor = true;
}

void ASLPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);

	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Sub->AddMappingContext(Mapping, 0);
	}
}

void ASLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_Sallanti"));
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogSLInput, Error, TEXT("EnhancedInputComponent yok: DefaultInput.ini DefaultInputComponentClass'a bak"));
		return;
	}

	auto Bind = [this, EIC](const TCHAR* Name, std::initializer_list<FKey> Keys, void (ASLPlayerController::*Handler)())
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		for (const FKey& Key : Keys)
		{
			Mapping->MapKey(Action, Key);
		}
		EIC->BindAction(Action, ETriggerEvent::Started, this, Handler);
	};
	Bind(TEXT("IA_Place"), { EKeys::LeftMouseButton }, &ASLPlayerController::OnPlace);
	Bind(TEXT("IA_Cancel"), { EKeys::RightMouseButton, EKeys::Escape }, &ASLPlayerController::OnCancel);
	Bind(TEXT("IA_Rotate"), { EKeys::R }, &ASLPlayerController::OnRotate);
	Bind(TEXT("IA_Slot1"), { EKeys::One }, &ASLPlayerController::OnSlot1);
	Bind(TEXT("IA_Slot2"), { EKeys::Two }, &ASLPlayerController::OnSlot2);
	Bind(TEXT("IA_Slot3"), { EKeys::Three }, &ASLPlayerController::OnSlot3);
	Bind(TEXT("IA_Slot4"), { EKeys::Four }, &ASLPlayerController::OnSlot4);
}

void ASLPlayerController::OnPlace() { Build->TryPlace(); }
void ASLPlayerController::OnCancel() { Build->Cancel(); }
void ASLPlayerController::OnRotate() { Build->RotateGhost(); }
void ASLPlayerController::OnSlot1() { Build->SelectSlot(0); }
void ASLPlayerController::OnSlot2() { Build->SelectSlot(1); }
void ASLPlayerController::OnSlot3() { Build->SelectSlot(2); }
void ASLPlayerController::OnSlot4() { Build->SelectSlot(3); }

void ASLPlayerController::SLGhost(int32 Slot, float LocalX, float LocalY)
{
	if (Build->SelectedSlot != Slot - 1 || Build->State == ESLBuildState::Idle)
	{
		Build->SelectSlot(Slot - 1);
	}
	Build->SetAimOverride(FVector2D(LocalX, LocalY));
	UE_LOG(LogSLInput, Log, TEXT("SL_GHOST slot=%d aim=(%.0f,%.0f) cell=(%d,%d) level=%d state=%s"),
		Slot, LocalX, LocalY, Build->GetGhostCellMin().X, Build->GetGhostCellMin().Y, Build->GetGhostLevel(),
		*UEnum::GetValueAsString(Build->State));
}

void ASLPlayerController::SLPlace(int32 Slot, float LocalX, float LocalY)
{
	SLGhost(Slot, LocalX, LocalY);
	const bool bPlaced = Build->TryPlace();
	UE_LOG(LogSLInput, Log, TEXT("SL_PLACE_RESULT %s count=%d"), bPlaced ? TEXT("OK") : TEXT("BLOCKED"), Build->PlacedCount);
}

void ASLPlayerController::SLMouse()
{
	Build->ClearAimOverride();
}

void ASLPlayerController::Shot(const TCHAR* Name)
{
	FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("SL_%s"), Name), false, false);
	UE_LOG(LogSLInput, Log, TEXT("SL_SHOT %s"), Name);
}

void ASLPlayerController::MouseAtLocal(int32 Slot, const FVector& Local)
{
	Build->SelectSlot(Slot - 1);
	Build->ClearAimOverride();
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLPlatform* P = GM ? GM->GetPlatform() : nullptr;
	FVector2D Screen;
	if (!P || !ProjectWorldLocationToScreen(P->TiltPivot->GetComponentTransform().TransformPosition(Local), Screen))
	{
		UE_LOG(LogSLInput, Error, TEXT("SL_MOUSE projeksiyon basarisiz"));
		return;
	}
	if (ULocalPlayer* LP = GetLocalPlayer(); LP && LP->ViewportClient && LP->ViewportClient->Viewport)
	{
		LP->ViewportClient->Viewport->SetMouse(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y));
	}
	UE_LOG(LogSLInput, Log, TEXT("SL_MOUSE local=%s screen=(%.0f,%.0f)"), *Local.ToString(), Screen.X, Screen.Y);
}

void ASLPlayerController::LogGhost(const TCHAR* Label)
{
	float MX = 0.f, MY = 0.f;
	GetMousePosition(MX, MY);
	UE_LOG(LogSLInput, Log, TEXT("SL_GHOST_MOUSE %s mouse=(%.0f,%.0f) cell=(%d,%d) level=%d state=%s"),
		Label, MX, MY, Build->GetGhostCellMin().X, Build->GetGhostCellMin().Y, Build->GetGhostLevel(),
		*UEnum::GetValueAsString(Build->State));
}

void ASLPlayerController::SLSelfTest()
{
	SelfTestIndex = 0;
	GetWorldTimerManager().SetTimer(SelfTestTimer, this, &ASLPlayerController::SelfTestStep, 0.5f, true, 1.0f);
}

void ASLPlayerController::SelfTestStep()
{
	// 0.5 sn'lik adimlar. Sallanma periyodu 4 sn: t=1 tepe (+), t=3 dip (-).
	const int32 I = SelfTestIndex++;
	SLReport();
	switch (I)
	{
	case 0: Shot(TEXT("1_kamera_platform")); break;
	case 2: Shot(TEXT("2a_sallanma")); break;
	case 6: Shot(TEXT("2b_sallanma")); break;
	case 9: SLGhost(1, 0.f, 0.f); break;
	case 10: Shot(TEXT("3_hayalet_merkez")); break;
	case 11: SLGhost(1, 37.f, 12.f); break;
	case 12: SLGhost(3, 290.f, 0.f); break;
	case 13: Shot(TEXT("4_hayalet_kenar")); break;
	case 14: SLGhost(3, 150.f, -150.f); break;
	case 15: Shot(TEXT("4b_hayalet_gecerli")); break;
	case 16: SLPlace(1, 225.f, 0.f); break;
	case 17: SLPlace(1, 225.f, 0.f); break;
	case 18: SLPlace(1, 225.f, 0.f); break;
	case 19: SLPlace(2, 275.f, 50.f); break;
	case 20: SLPlace(3, 400.f, 0.f); break; // disarida: BLOCKED beklenir
	case 21: Build->Cancel(); Shot(TEXT("5_kule")); break;
	case 26: Shot(TEXT("6_egim")); break;
	case 28: SLPlace(4, -225.f, 0.f); break; // karsi tarafa agir bina: egim donmeli
	case 33: Shot(TEXT("6b_karsi_agirlik")); break;
	// Gercek fare yolu: imleci bilinen bir yerel noktanin ekran konumuna koy, isin kodu bulsun.
	case 34: MouseAtLocal(1, FVector(60.f, -190.f, 0.f)); break;          // kameraya yakin, onu bos: beklenen (7,2) seviye 0
	case 35: LogGhost(TEXT("fare_zemin")); Shot(TEXT("7a_fare_zemin")); break;
	case 36: MouseAtLocal(1, FVector(225.f, 25.f, 150.f)); break;          // kulenin ustu: (10,6) seviye 3
	case 37: LogGhost(TEXT("fare_kule_ustu")); Shot(TEXT("7b_fare_kule_ustu")); break;
	case 38: OnPlace(); break;                                              // sol tik yolu
	case 39: SLReport(); Shot(TEXT("7c_fare_yerlesti")); break;
	case 41:
		GetWorldTimerManager().ClearTimer(SelfTestTimer);
		UE_LOG(LogSLInput, Log, TEXT("SL_SELFTEST_DONE"));
		ConsoleCommand(TEXT("quit"));
		break;
	default: break;
	}
}

void ASLPlayerController::SLReport()
{
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLPlatform* P = GM ? GM->GetPlatform() : nullptr;
	if (!P)
	{
		UE_LOG(LogSLInput, Error, TEXT("SL_REPORT platform yok"));
		return;
	}
	const float H = P->HalfExtent;
	UE_LOG(LogSLInput, Log,
		TEXT("SL_REPORT t=%.2f com=(%.1f,%.1f) mass=%.1f tilt=(%.2f,%.2f) total=%.2f | Z +X=%.1f -X=%.1f +Y=%.1f -Y=%.1f | placed=%d"),
		GetWorld()->GetTimeSeconds(), P->CenterOfMass.X, P->CenterOfMass.Y, P->TotalMass,
		P->CurrentTilt.X, P->CurrentTilt.Y, P->CurrentTiltDegrees,
		P->GetWorldZOfLocal(FVector(H, 0, 0)), P->GetWorldZOfLocal(FVector(-H, 0, 0)),
		P->GetWorldZOfLocal(FVector(0, H, 0)), P->GetWorldZOfLocal(FVector(0, -H, 0)),
		Build->PlacedCount);
}
