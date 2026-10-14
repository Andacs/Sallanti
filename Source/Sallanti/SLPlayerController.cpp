#include "SLPlayerController.h"

#include "SLBuildComponent.h"
#include "SLCameraPawn.h"
#include "SLGameMode.h"
#include "SLPlatform.h"
#include "SLTestHUD.h"
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
	Bind(TEXT("IA_Grid"), { EKeys::G }, &ASLPlayerController::OnToggleGrid);
	Bind(TEXT("IA_Help"), { EKeys::H }, &ASLPlayerController::OnToggleHelp);
}

void ASLPlayerController::OnUndo() { Build->UndoLast(); }
void ASLPlayerController::OnRestart() { RestartLevel(); }

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
	if (ASLTestHUD* HUD = GetHUD<ASLTestHUD>())
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

void ASLPlayerController::SLTest7()
{
	SelfTestIndex = 0;
	// Kademeli olsun diye platform kutlesi: tek bina esigi hemen gecmesin.
	SLSet(TEXT("PlatformMass"), TEXT("12"));
	SLSet(TEXT("SwayAmplitude"), TEXT("0")); // esik gecisini sallanma karistirmasin
	GetWorldTimerManager().SetTimer(SelfTestTimer, this, &ASLPlayerController::Test7Step, 0.5f, true, 1.0f);
}

void ASLPlayerController::Test7Step()
{
	const int32 I = SelfTestIndex++;
	SLReport();
	// 1-3: -X tarafinda 3 kutuluk kule (devrilmesi gorulsun). 4-9: +X kenarina 12'lik binalar.
	// Beklenen hedef egim (PlatformMass 12, kule -125 cm'de 3 kutle): 3.9 / 5.8 / 6.8 / 7.4 / 7.8 / 8.1
	// -> esik (8.0) ancak 6. binada gecilir.
	switch (I)
	{
	case 1: case 2: case 3: SLPlace(1, -110.f, 10.f); break;
	case 4: SLPlace(4, 250.f, -250.f); break;
	case 5: SLPlace(4, 250.f, -150.f); break;
	case 6: SLPlace(4, 250.f, -50.f); break;
	case 7: SLPlace(4, 250.f, 50.f); break;
	case 8: SLPlace(4, 250.f, 150.f); break;
	case 9: Shot(TEXT("T7_0_esik_oncesi")); break;
	case 10: SLPlace(4, 250.f, 250.f); Build->Cancel(); break;
	case 11: Shot(TEXT("T7_1_birakma_05sn")); break;
	case 13: Shot(TEXT("T7_2_15sn")); break;
	case 16: Shot(TEXT("T7_3_3sn")); break;
	case 20: Shot(TEXT("T7_4_5sn")); break;
	case 28: Shot(TEXT("T7_5_9sn")); break;
	case 30:
		GetWorldTimerManager().ClearTimer(SelfTestTimer);
		UE_LOG(LogSLInput, Log, TEXT("SL_TEST7_DONE"));
		ConsoleCommand(TEXT("quit"));
		break;
	default: break;
	}
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
	// Test paneli araclari: canli ayar ve geri alma.
	case 40: SLKnobs(); SLSet(TEXT("PlatformMass"), TEXT("10")); break;  // CoM = Sum(m*r)/(Sum(m)+10) kuculmeli
	case 42: OnUndo(); break;                                             // kule tepesindeki kutu gider
	case 44: Shot(TEXT("8_test_paneli")); break;
	case 46:
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
		TEXT("SL_REPORT t=%.2f com=(%.1f,%.1f) mass=%.1f tilt=(%.2f,%.2f) target=%.2f total=%.2f | Z +X=%.1f -X=%.1f +Y=%.1f -Y=%.1f | placed=%d released=%d ondeck=%d fallen=%d"),
		GetWorld()->GetTimeSeconds(), P->CenterOfMass.X, P->CenterOfMass.Y, P->TotalMass,
		P->CurrentTilt.X, P->CurrentTilt.Y, P->TargetTiltDegrees, P->CurrentTiltDegrees,
		P->GetWorldZOfLocal(FVector(H, 0, 0)), P->GetWorldZOfLocal(FVector(-H, 0, 0)),
		P->GetWorldZOfLocal(FVector(0, H, 0)), P->GetWorldZOfLocal(FVector(0, -H, 0)),
		Build->PlacedCount, P->ReleasedCount, P->ReleasedOnDeck, P->FallenCount);
}
