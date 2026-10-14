#include "SLPlayerController.h"

#include "SLBuildComponent.h"
#include "SLCameraPawn.h"
#include "SLGameMode.h"
#include "SLLevelDirector.h"
#include "SLPlatform.h"
#include "SLHUD.h"
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
	Bind(TEXT("IA_Undo"), { EKeys::BackSpace }, &ASLPlayerController::OnUndo);
	Bind(TEXT("IA_Restart"), { EKeys::F5 }, &ASLPlayerController::OnRestart);
	Bind(TEXT("IA_Again"), { EKeys::SpaceBar }, &ASLPlayerController::OnAgain);
	Bind(TEXT("IA_Grid"), { EKeys::G }, &ASLPlayerController::OnToggleGrid);
	Bind(TEXT("IA_Help"), { EKeys::H }, &ASLPlayerController::OnToggleHelp);
}

void ASLPlayerController::OnUndo() { Build->UndoLast(); }
void ASLPlayerController::OnRestart() { RestartLevel(); }

void ASLPlayerController::OnAgain()
{
	// Space yalnizca oyun bittiyse yeniden baslatir; oynarken yanlislikla basilirsa bir sey olmaz.
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	if (GM && GM->IsRoundOver())
	{
		RestartLevel();
	}
}

void ASLPlayerController::OnToggleGrid()
{
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	if (ASLPlatform* P = GM ? GM->GetPlatform() : nullptr)
	{
		P->bGridAlwaysVisible = !P->bGridAlwaysVisible;
		P->SetGridShownForPlacing(Build->State != ESLBuildState::Idle);
	}
}

void ASLPlayerController::OnToggleHelp()
{
	if (ASLHUD* HUD = GetHUD<ASLHUD>())
	{
		HUD->bShowTestPanel = !HUD->bShowTestPanel;
	}
}

TArray<UObject*> ASLPlayerController::GetKnobTargets() const
{
	TArray<UObject*> Targets;
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	if (ASLPlatform* P = GM ? GM->GetPlatform() : nullptr)
	{
		Targets.Add(P);
	}
	Targets.Add(Build);
	if (APawn* CamPawn = GetPawn())
	{
		Targets.Add(CamPawn);
	}
	return Targets;
}

static bool IsKnob(const FProperty* Prop)
{
	return Prop->HasAnyPropertyFlags(CPF_Edit) && !Prop->HasAnyPropertyFlags(CPF_EditConst)
		&& (Prop->IsA<FFloatProperty>() || Prop->IsA<FDoubleProperty>() || Prop->IsA<FIntProperty>() || Prop->IsA<FBoolProperty>());
}

void ASLPlayerController::SLSet(const FString& Name, const FString& Value)
{
	for (UObject* Target : GetKnobTargets())
	{
		FProperty* Prop = FindFProperty<FProperty>(Target->GetClass(), FName(*Name));
		if (!Prop || !IsKnob(Prop))
		{
			continue;
		}
		void* Ptr = Prop->ContainerPtrToValuePtr<void>(Target);
		FString Before;
		Prop->ExportTextItem_Direct(Before, Ptr, nullptr, Target, PPF_None);
		if (!Prop->ImportText_Direct(*Value, Ptr, Target, PPF_None))
		{
			ClientMessage(FString::Printf(TEXT("SLSet: '%s' %s icin gecerli bir deger degil"), *Value, *Name));
			return;
		}
		FString After;
		Prop->ExportTextItem_Direct(After, Ptr, nullptr, Target, PPF_None);
		if (ASLPlatform* P = Cast<ASLPlatform>(Target))
		{
			P->RefreshVisuals();
		}
		else if (ASLCameraPawn* Cam = Cast<ASLCameraPawn>(Target))
		{
			Cam->ApplyCamera();
		}
		const FString Msg = FString::Printf(TEXT("SLSet %s.%s: %s -> %s"), *Target->GetClass()->GetName(), *Name, *Before, *After);
		ClientMessage(Msg);
		UE_LOG(LogSLInput, Log, TEXT("%s"), *Msg);
		return;
	}
	ClientMessage(FString::Printf(TEXT("SLSet: '%s' diye bir ayar yok. SLKnobs ile listele."), *Name));
}

void ASLPlayerController::SLKnobs()
{
	for (UObject* Target : GetKnobTargets())
	{
		for (TFieldIterator<FProperty> It(Target->GetClass()); It; ++It)
		{
			if (!IsKnob(*It) || !It->GetOwnerClass()->GetName().StartsWith(TEXT("SL")))
			{
				continue; // motorun kendi ayarlarini degil, sadece bizimkileri
			}
			FString Val;
			It->ExportTextItem_Direct(Val, It->ContainerPtrToValuePtr<void>(Target), nullptr, Target, PPF_None);
			const FString Msg = FString::Printf(TEXT("%s.%s = %s"), *Target->GetClass()->GetName(), *It->GetName(), *Val);
			ClientMessage(Msg);
			UE_LOG(LogSLInput, Log, TEXT("SL_KNOB %s"), *Msg);
		}
	}
}

void ASLPlayerController::OnPlace()
{
	// Elde bir sey yoksa sol tik tasima baslatir: tiklanan nesne kaldirilir.
	if (Build->State == ESLBuildState::Idle && !Build->IsMoving())
	{
		Build->TryPickUpUnderCursor();
		return;
	}
	Build->TryPlace();
}
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
	if (Build->SelectedSlot != Slot - 1)
	{
		// Kilitli slot: secim reddedildi. Onceki slotla devam etmesin.
		Build->Cancel();
		UE_LOG(LogSLInput, Log, TEXT("SL_GHOST slot=%d KILITLI"), Slot);
		return;
	}
	Build->SetAimOverride(FVector2D(LocalX, LocalY));
	UE_LOG(LogSLInput, Log, TEXT("SL_GHOST slot=%d aim=(%.0f,%.0f) cell=(%d,%d) level=%d state=%s"),
		Slot, LocalX, LocalY, Build->GetGhostCellMin().X, Build->GetGhostCellMin().Y, Build->GetGhostLevel(),
		*UEnum::GetValueAsString(Build->State));
}

void ASLPlayerController::SLPlace(int32 Slot, float LocalX, float LocalY)
{
	SLGhost(Slot, LocalX, LocalY);
	const bool bPlaced = Build->SelectedSlot == Slot - 1 && Build->TryPlace();
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
	MoveMouseToLocal(Local);
}

void ASLPlayerController::MoveMouseToLocal(const FVector& Local)
{
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

void ASLPlayerController::SLTestLevel1()
{
	SelfTestIndex = 0;
	SLSet(TEXT("SwayAmplitude"), TEXT("0")); // esik gecislerini sallanma karistirmasin
	GetWorldTimerManager().SetTimer(SelfTestTimer, this, &ASLPlayerController::TestLevel1Step, 0.5f, true, 1.0f);
}

void ASLPlayerController::TestLevel1Step()
{
	const int32 I = SelfTestIndex++;
	SLReport();
	// Beklenen (PlatformMass 12, TiltPerCm 0.08): Ev (-100,0) 3.2 | Hastane (150,0) 2.5 |
	// AVM (175,225) hedef 12 -> DEVRILIYOR | AVM'yi fareyle kaldir -> 2.5 | (25,25)'e koy -> 2.5 |
	// yol (3..8, 4) -> 6. karoda uc bina bagli -> BOLUM TAMAMLANDI.
	switch (I)
	{
	case 1: SLPlace(1, -100.f, 0.f); break;                  // Ev
	case 2: Shot(TEXT("L1_1_aile_yuruyor")); SLPlace(2, 150.f, 0.f); break; // Hastane henuz kilitli
	case 8: Shot(TEXT("L1_2_aile_yerlesti")); SLPlace(2, 150.f, 0.f); break; // Hastane
	case 9: SLPlace(3, 150.f, 225.f); break;                 // AVM yanlis yere
	case 10: Build->Cancel(); MoveMouseToLocal(FVector(175.f, 225.f, 200.f)); Shot(TEXT("L1_3_devriliyor")); break;
	case 11: OnPlace(); LogGhost(TEXT("tasima_fare")); break; // bosta sol tik: AVM'yi kaldir
	case 12: Build->SetAimOverride(FVector2D(25.f, 25.f)); Shot(TEXT("L1_4_elde")); break;
	case 13: OnPlace(); break;                               // yeni yerine
	case 14: Shot(TEXT("L1_5_tasindi")); break;
	case 15: case 16: case 17: case 18: case 19: case 20:
		SLPlace(4, -300.f + (3 + (I - 15) + 0.5f) * 50.f, -75.f); break; // yol hucre (3..8, 4)
	case 22: Shot(TEXT("L1_6_tamamlandi")); break;
	case 24:
		GetWorldTimerManager().ClearTimer(SelfTestTimer);
		UE_LOG(LogSLInput, Log, TEXT("SL_TESTL1_DONE"));
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
		TEXT("SL_REPORT t=%.2f com=(%.1f,%.1f) mass=%.1f tilt=(%.2f,%.2f) target=%.2f total=%.2f | Z +X=%.1f -X=%.1f +Y=%.1f -Y=%.1f | placed=%d state=%s failtimer=%.2f"),
		GetWorld()->GetTimeSeconds(), P->CenterOfMass.X, P->CenterOfMass.Y, P->TotalMass,
		P->CurrentTilt.X, P->CurrentTilt.Y, P->TargetTiltDegrees, P->CurrentTiltDegrees,
		P->GetWorldZOfLocal(FVector(H, 0, 0)), P->GetWorldZOfLocal(FVector(-H, 0, 0)),
		P->GetWorldZOfLocal(FVector(0, H, 0)), P->GetWorldZOfLocal(FVector(0, -H, 0)),
		Build->PlacedCount, *UEnum::GetValueAsString(P->BalanceState), P->FailTimer);
	if (const ASLLevelDirector* D = GM->GetDirector())
	{
		UE_LOG(LogSLInput, Log, TEXT("SL_TASK %d/%d progress %d/%d complete=%d moving=%d"),
			D->CurrentTask + 1, D->Tasks.Num(), D->Progress, D->ProgressGoal, D->bLevelComplete ? 1 : 0, Build->IsMoving() ? 1 : 0);
	}
}
