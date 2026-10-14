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
#include "InputActionValue.h"
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

	auto Bind = [this, EIC](const TCHAR* Name, std::initializer_list<FKey> Keys, void (ASLPlayerController::*Handler)()) -> UInputAction*
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		for (const FKey& Key : Keys)
		{
			Mapping->MapKey(Action, Key);
		}
		EIC->BindAction(Action, ETriggerEvent::Started, this, Handler);
		return Action;
	};
	UInputAction* PlaceAction = Bind(TEXT("IA_Place"), { EKeys::LeftMouseButton }, &ASLPlayerController::OnPlace);
	// Ayni eylemin birakilisi: cekmece surukleme biter.
	EIC->BindAction(PlaceAction, ETriggerEvent::Completed, this, &ASLPlayerController::OnPlaceReleased);
	Bind(TEXT("IA_Drawer"), { EKeys::Tab }, &ASLPlayerController::OnToggleDrawer);

	// Tekerlek: eksen eylemi (deger = tik, + ileri).
	UInputAction* ZoomAction = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;
	Mapping->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ASLPlayerController::OnZoom);
	Bind(TEXT("IA_Cancel"), { EKeys::Escape }, &ASLPlayerController::OnCancel);
	// Sag tik: basili tutup surukle = haritayi kaydir; surukleme olmadan birak = elindekini birak (iptal).
	UInputAction* PanAction = Bind(TEXT("IA_Pan"), { EKeys::RightMouseButton }, &ASLPlayerController::OnPanPressed);
	EIC->BindAction(PanAction, ETriggerEvent::Completed, this, &ASLPlayerController::OnPanReleased);
	Bind(TEXT("IA_CenterCam"), { EKeys::C }, &ASLPlayerController::OnCenterCamera);
	Bind(TEXT("IA_Rotate"), { EKeys::R }, &ASLPlayerController::OnRotate);
	Bind(TEXT("IA_RotateBack"), { EKeys::Q }, &ASLPlayerController::OnRotateBack);
	Bind(TEXT("IA_Slot1"), { EKeys::One }, &ASLPlayerController::OnSlot1);
	Bind(TEXT("IA_Slot2"), { EKeys::Two }, &ASLPlayerController::OnSlot2);
	Bind(TEXT("IA_Slot3"), { EKeys::Three }, &ASLPlayerController::OnSlot3);
	Bind(TEXT("IA_Slot4"), { EKeys::Four }, &ASLPlayerController::OnSlot4);
	Bind(TEXT("IA_Undo"), { EKeys::BackSpace }, &ASLPlayerController::OnUndo);
	Bind(TEXT("IA_Restart"), { EKeys::F5 }, &ASLPlayerController::OnRestart);
	Bind(TEXT("IA_Again"), { EKeys::SpaceBar }, &ASLPlayerController::OnAgain);
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
			Cam->SetTargetDistance(Cam->Distance); // Distance degistiyse yumusatma geri cekmesin
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

void ASLPlayerController::OnPanPressed()
{
	bPanning = true;
	PanMoved = 0.f;
	GetMousePosition(LastPanMouse.X, LastPanMouse.Y);
}

void ASLPlayerController::OnPanReleased()
{
	bPanning = false;
	if (PanMoved < PanClickThresholdPx)
	{
		Build->Cancel(); // surukleme degil tiklama: eskisi gibi iptal
	}
}

void ASLPlayerController::OnCenterCamera()
{
	if (ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetPawn()))
	{
		Cam->ResetPan();
	}
}

void ASLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!bPanning)
	{
		return;
	}
	FVector2D Mouse;
	if (!GetMousePosition(Mouse.X, Mouse.Y))
	{
		return;
	}
	const FVector2D D = Mouse - LastPanMouse;
	LastPanMouse = Mouse;
	PanMoved += D.Size();
	if (PanMoved >= PanClickThresholdPx && !D.IsNearlyZero())
	{
		int32 VX = 0, VY = 0;
		GetViewportSize(VX, VY);
		if (ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetPawn()))
		{
			Cam->PanByScreen(D.X, D.Y, static_cast<float>(VX));
		}
	}
}

void ASLPlayerController::OnPlaceReleased()
{
	float MX = 0.f, MY = 0.f;
	GetMousePosition(MX, MY);
	if (ASLHUD* HUD = GetHUD<ASLHUD>())
	{
		HUD->HandleMouseUp(FVector2D(MX, MY));
	}
}

void ASLPlayerController::OnZoom(const FInputActionValue& Value)
{
	if (ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetPawn()))
	{
		Cam->Zoom(Value.Get<float>());
	}
}

void ASLPlayerController::OnToggleDrawer()
{
	if (ASLHUD* HUD = GetHUD<ASLHUD>())
	{
		HUD->ToggleDrawer();
	}
}

void ASLPlayerController::OnPlace()
{
	// Once arayuz: cekmece ve kulpu tiklamayi yakalar, oyuna gecmez.
	float MX = 0.f, MY = 0.f;
	if (GetMousePosition(MX, MY))
	{
		if (ASLHUD* HUD = GetHUD<ASLHUD>(); HUD && HUD->HandleMouseDown(FVector2D(MX, MY)))
		{
			return;
		}
	}
	// Elde bir sey yoksa sol tik tasima baslatir: tiklanan nesne kaldirilir.
	if (Build->State == ESLBuildState::Idle && !Build->IsMoving())
	{
		Build->TryPickUpUnderCursor();
		return;
	}
	Build->TryPlace();
}
void ASLPlayerController::OnCancel() { Build->Cancel(); }
void ASLPlayerController::OnRotate() { Build->RotateGhost(1); }
void ASLPlayerController::OnRotateBack() { Build->RotateGhost(-1); }
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
	UE_LOG(LogSLInput, Log, TEXT("SL_GHOST_MOUSE %s mouse=(%.0f,%.0f) cell=(%d,%d) level=%d yon=%d state=%s"),
		Label, MX, MY, Build->GetGhostCellMin().X, Build->GetGhostCellMin().Y, Build->GetGhostLevel(),
		Build->GetGhostFacing(), *UEnum::GetValueAsString(Build->State));
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
	ASLHUD* HUD = GetHUD<ASLHUD>();
	ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetPawn());
	if (HUD)
	{
		UE_LOG(LogSLInput, Log, TEXT("SL_HINT %s"), *HUD->GetCurrentHint());
	}
	// 13x13 tahta, 50 cm hucre (CellToLocal(i) = -325 + 50i). Beklenen (PlatformMass 12, TiltPerCm 0.08):
	//  Ev 1x1 (-100,0) hucre (4,6) yon 3, giris (4,5); egim 1.1 | aile (-230,-250)'den kapiya ~4 sn |
	//  Hastane 2x2 (175,25) hucre (9,6) yon 1 (kapi +Y, yanlis) giris (9,8)(10,8); egim 5.9 |
	//  AVM 3x3 (250,250) hucre (10,10): hedef 12 -> DEVRILIYOR -> fareyle kaldir -> 5.9 -> (0,50) hucre (5,6)
	//  giris (6,5); egim 3.7 | yol (4..9, 5): ilk karoda Ev baglanir (efekt), (6,5)'te AVM; Hastane kapisi
	//  ters: 2/3 | Hastaneyi kaldir, R R (1 -> 3), ayni yere: giris (9,5)(10,5) -> 3/3 BOLUM TAMAMLANDI.
	switch (I)
	{
	case 1: Shot(TEXT("L1_0_baslangic")); break;
	case 3: if (HUD) { HUD->HandleMouseDown(HUD->GetHandleCenter()); HUD->HandleMouseUp(HUD->GetHandleCenter()); }
		UE_LOG(LogSLInput, Log, TEXT("SL_DRAWER kulpa tik -> istenen acik=%d"), HUD && HUD->IsDrawerWanted() ? 1 : 0); break;
	case 4: Shot(TEXT("L1_0b_cekmece_acik")); break;
	case 5: if (HUD) { HUD->HandleMouseDown(HUD->GetCardCenter(0)); }
		UE_LOG(LogSLInput, Log, TEXT("SL_DRAWER Ev kartina tik -> secilen slot %d, istenen acik %d"), Build->SelectedSlot, HUD && HUD->IsDrawerWanted() ? 1 : 0); break;
	// Ekran goruntusu kare sonunda alinir: hayaleti gostereceksek koyma bir sonraki adimda olmali.
	case 6: Build->SetAimOverride(FVector2D(-100.f, 0.f)); Shot(TEXT("L1_0c_ev_hayalet")); break;
	case 7: OnPlace(); break;                                                       // Ev, yon 3
	case 8: SLPlace(2, 175.f, 25.f); break;                                         // Hastane henuz kilitli
	case 9: Shot(TEXT("L1_1_aile_yuruyor")); break;
	case 13: Shot(TEXT("L1_1b_aile_yolda")); break;
	case 18: Shot(TEXT("L1_2_aile_yerlesti")); if (Cam) { Cam->Zoom(3.f); } break;
	case 20: Shot(TEXT("L1_2b_yakin")); UE_LOG(LogSLInput, Log, TEXT("SL_ZOOM uzaklik %.0f hedef %.0f"), Cam ? Cam->Distance : 0.f, Cam ? Cam->GetTargetDistance() : 0.f);
		if (Cam) { Cam->Zoom(-3.f); } break;
	case 22: SLGhost(2, 175.f, 25.f); Build->SetFacing(1); Shot(TEXT("L1_2c_hastane_ters")); break;
	case 23: Build->TryPlace(); break;                                              // Hastane, kapi +Y
	case 24: SLGhost(3, 250.f, 250.f); Shot(TEXT("L1_3a_avm_tahmin")); break;      // tahmin isareti + ipucu
	case 25: Build->TryPlace(); break;                                              // AVM yanlis yere
	case 26: Build->Cancel(); MoveMouseToLocal(FVector(250.f, 250.f, 204.f)); Shot(TEXT("L1_3_devriliyor")); break;
	case 27:
		OnPlace(); LogGhost(TEXT("tasima_fare"));                                     // bosta sol tik: AVM'yi kaldir
		if (!Build->IsMoving())
		{
			UE_LOG(LogSLInput, Warning, TEXT("SL_TASIMA fare kaydi (kullanici?), hucreden kaldiriliyor"));
			Build->PickUp(GetWorld()->GetAuthGameMode<ASLGameMode>()->GetPlatform()->FindPlaceableAt(FIntPoint(11, 11)));
		}
		break;
	case 28: Build->SetAimOverride(FVector2D(0.f, 50.f)); Shot(TEXT("L1_4_elde")); break;
	case 29: OnPlace(); break;                                                      // yeni yerine
	case 30: case 31: case 32: case 33: case 34: case 35:
		SLPlace(4, 50.f * (4 + (I - 30)) - 300.f, -50.f);                           // yol hucre (4..9, 5)
		if (I == 31) { Shot(TEXT("L1_5a_ev_baglandi")); }
		if (I == 33) { Shot(TEXT("L1_5b_avm_baglandi")); }
		break;
	case 36: Build->Cancel(); Shot(TEXT("L1_5c_hastane_bagli_degil")); break;
	case 37: Build->PickUp(GetWorld()->GetAuthGameMode<ASLGameMode>()->GetPlatform()->FindPlaceableAt(FIntPoint(9, 6))); break;
	case 38: OnRotate(); OnRotate(); Build->SetAimOverride(FVector2D(175.f, 25.f)); LogGhost(TEXT("hastane_R_R")); break;
	case 39: Shot(TEXT("L1_6_hastane_cevrildi")); break;
	case 40: OnPlace(); break;
	case 41: Shot(TEXT("L1_6b_hastane_baglandi")); break;
	case 43: Shot(TEXT("L1_7_tamamlandi")); if (Cam) { Cam->PanByScreen(-300.f, 0.f, 1280.f); }
		UE_LOG(LogSLInput, Log, TEXT("SL_PAN kaydirma (%.0f, %.0f)"), Cam ? Cam->GetPanOffset().X : 0.f, Cam ? Cam->GetPanOffset().Y : 0.f); break;
	case 44: Shot(TEXT("L1_8_kaydirildi")); OnCenterCamera();
		UE_LOG(LogSLInput, Log, TEXT("SL_PAN merkez (%.0f, %.0f)"), Cam ? Cam->GetPanOffset().X : 0.f, Cam ? Cam->GetPanOffset().Y : 0.f); break;
	case 45: if (Cam) { Cam->Zoom(7.f); } break;                                   // yakin plan: viraj ve giris yolu
	case 47: Shot(TEXT("L1_9_yakin_yol")); break;
	case 49:
		GetWorldTimerManager().ClearTimer(SelfTestTimer);
		UE_LOG(LogSLInput, Log, TEXT("SL_TESTL1_DONE"));
		ConsoleCommand(TEXT("quit"));
		break;
	default: break;
	}
}

void ASLPlayerController::MoveMouseToScreen(const FVector2D& Screen)
{
	Build->ClearAimOverride();
	if (ULocalPlayer* LP = GetLocalPlayer(); LP && LP->ViewportClient && LP->ViewportClient->Viewport)
	{
		LP->ViewportClient->Viewport->SetMouse(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y));
	}
	UE_LOG(LogSLInput, Log, TEXT("SL_MOUSE ekran=(%.0f,%.0f)"), Screen.X, Screen.Y);
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
