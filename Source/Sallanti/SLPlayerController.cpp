#include "SLPlayerController.h"

#include "SLBuildComponent.h"
#include "SLCameraPawn.h"
#include "SLGameMode.h"
#include "SLLevelDirector.h"
#include "SLPlatform.h"
#include "SLPlaceable.h"
#include "SLHUD.h"
#include "SLAmbience.h"
#include "EngineUtils.h"
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
	Bind(TEXT("IA_Slot5"), { EKeys::Five }, &ASLPlayerController::OnSlot5);
	Bind(TEXT("IA_Slot6"), { EKeys::Six }, &ASLPlayerController::OnSlot6);
	Bind(TEXT("IA_Music"), { EKeys::M }, &ASLPlayerController::OnToggleMusic);
	Bind(TEXT("IA_ViewLeft"), { EKeys::Z }, &ASLPlayerController::OnRotateViewLeft);
	Bind(TEXT("IA_ViewRight"), { EKeys::X }, &ASLPlayerController::OnRotateViewRight);
	Bind(TEXT("IA_Next"), { EKeys::Enter }, &ASLPlayerController::OnNextLevel);
	Bind(TEXT("IA_Undo"), { EKeys::BackSpace }, &ASLPlayerController::OnUndo);
	Bind(TEXT("IA_Restart"), { EKeys::F5 }, &ASLPlayerController::OnRestart);
	Bind(TEXT("IA_Again"), { EKeys::SpaceBar }, &ASLPlayerController::OnAgain);
	Bind(TEXT("IA_Help"), { EKeys::H }, &ASLPlayerController::OnToggleHelp);
}

void ASLPlayerController::OnUndo() { Build->UndoLast(); }
void ASLPlayerController::OnRestart()
{
	// Ayni bolumu bastan (RestartLevel URL secenegini korumayabilir; bolum numarasini acikca ver).
	if (ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>())
	{
		GM->TravelToLevel(GM->LevelNumber);
	}
}

void ASLPlayerController::OnAgain()
{
	// Space yalnizca oyun bittiyse yeniden baslatir; oynarken yanlislikla basilirsa bir sey olmaz.
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	if (GM && GM->IsRoundOver())
	{
		OnRestart();
	}
}

void ASLPlayerController::OnToggleHelp()
{
	if (ASLHUD* HUD = GetHUD<ASLHUD>())
	{
		HUD->bShowTestPanel = !HUD->bShowTestPanel;
		if (ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>(); GM && GM->GetPlatform())
		{
			GM->GetPlatform()->bDebugDraw = HUD->bShowTestPanel; // CoM kuresi yalniz panel acikken
		}
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
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	ASLLevelDirector* Dir = GM ? GM->GetDirector() : nullptr;
	const int32 Fam = Dir ? Dir->GetSelectedFamily() : INDEX_NONE;
	// Elde bir sey yoksa sol tik tasima baslatir: tiklanan nesne kaldirilir. Aile seciliyken bos eve
	// tiklamak aileyi o eve tasir (kaldirmaz).
	if (Build->State == ESLBuildState::Idle && !Build->IsMoving())
	{
		if (Fam != INDEX_NONE)
		{
			ASLPlaceable* Under = Build->GetPlaceableUnderCursor();
			if (Under && Under->Residents > 0 && Dir->GetHome(Fam) != Under && Dir->AssignFamilyToHouse(Fam, Under))
			{
				Dir->ClearFamilySelection();
				return;
			}
		}
		Build->TryPickUpUnderCursor();
		return;
	}
	const bool bWasMoving = Build->IsMoving();
	const int32 Before = Build->PlacedCount;
	if (Build->TryPlace() && !bWasMoving && Build->PlacedCount > Before && Fam != INDEX_NONE)
	{
		// Aile seciliyken ev kuruldu: o aile tasinir.
		ASLPlaceable* New = Build->GetLastPlaced();
		if (New && New->Residents > 0 && Dir->AssignFamilyToHouse(Fam, New))
		{
			Dir->ClearFamilySelection();
		}
	}
}

void ASLPlayerController::OnRotateViewLeft() { RotateView(-1); }
void ASLPlayerController::OnRotateViewRight() { RotateView(1); }

void ASLPlayerController::RotateView(int32 Steps)
{
	if (ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetPawn()))
	{
		Cam->RotateView(Steps);
		// Yeni binanin kapisi kameraya baksin: ilk bakista -Y (3), her ceyrek turda bir yon.
		Build->DefaultFacing = (3 + Cam->GetViewSteps()) & 3;
		ASLGameMode::PlaySfx(this, TEXT("SFX_Cekmece"), 0, 0.6f, 1.2f);
	}
}
void ASLPlayerController::OnCancel()
{
	Build->Cancel();
	if (ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>(); GM && GM->GetDirector())
	{
		GM->GetDirector()->ClearFamilySelection();
	}
}
void ASLPlayerController::OnRotate() { Build->RotateGhost(1); }
void ASLPlayerController::OnRotateBack() { Build->RotateGhost(-1); }
void ASLPlayerController::OnSlot1() { Build->SelectSlot(0); }
void ASLPlayerController::OnSlot2() { Build->SelectSlot(1); }
void ASLPlayerController::OnSlot3() { Build->SelectSlot(2); }
void ASLPlayerController::OnSlot4() { Build->SelectSlot(3); }
void ASLPlayerController::OnSlot5() { Build->SelectSlot(4); }
void ASLPlayerController::OnSlot6() { Build->SelectSlot(5); }

void ASLPlayerController::OnToggleMusic()
{
	if (ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>())
	{
		GM->ToggleMusic();
	}
}

void ASLPlayerController::SLSaat(float Hour)
{
	for (TActorIterator<ASLAmbience> It(GetWorld()); It; ++It)
	{
		It->DebugHourOverride = Hour;
	}
}

void ASLPlayerController::OnNextLevel()
{
	// Enter yalnizca bolum bittiyse ve sonraki bolum varsa.
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
	if (GM && D && GM->IsLevelComplete() && D->NextLevelNumber > 0)
	{
		GM->TravelToLevel(D->NextLevelNumber);
	}
}

void ASLPlayerController::SLBolum(int32 Number)
{
	if (ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>())
	{
		GM->TravelToLevel(Number);
	}
}

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

void ASLPlayerController::SLTestLevel2()
{
	SelfTestIndex = 0;
	SLSet(TEXT("SwayAmplitude"), TEXT("0"));
	GetWorldTimerManager().SetTimer(SelfTestTimer, this, &ASLPlayerController::TestLevel2Step, 0.5f, true, 1.0f);
}

void ASLPlayerController::TestLevel2Step()
{
	const int32 I = SelfTestIndex++;
	SLReport();
	ASLHUD* HUD = GetHUD<ASLHUD>();
	ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetPawn());
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
	const ASLPlatform* Plat = GM ? GM->GetPlatform() : nullptr;
	if (HUD)
	{
		UE_LOG(LogSLInput, Log, TEXT("SL_HINT %s"), *HUD->GetCurrentHint());
	}
	if (D)
	{
		for (int32 F = 0; F < D->Families.Num(); ++F)
		{
			FString Bubble;
			D->GetFamilyBubble(F, Bubble);
			UE_LOG(LogSLInput, Log, TEXT("SL_FAMILY_STATE %s mutluluk %.2f (%s) balon: %s"), *D->Families[F].Name, D->GetHappiness(F), *D->GetFamilyStatus(F), *Bubble);
		}
		UE_LOG(LogSLInput, Log, TEXT("SL_TIME saat %.1f hamle %d kalan %d"), D->GetTimeHours(), D->GetMoveCount(), D->GetMovesLeft());
	}
	// Gercek fare bu makinede guvenilmez (kullanici oynatiyor): tiklamalarin yaptigini dogrudan cagir.
	// Aileye tiklama = SelectFamily; aile seciliyken ev kurma = AssignFamilyToHouse (OnPlace ayni seyi yapar).
	auto Place = [this](int32 Slot, float X, float Y, int32 Facing)
	{
		SLGhost(Slot, X, Y);
		Build->SetFacing(Facing);
		const bool bOk = Build->TryPlace();
		UE_LOG(LogSLInput, Log, TEXT("SL_PLACE_RESULT %s slot %d (%.0f,%.0f) yon %d"), bOk ? TEXT("OK") : TEXT("BLOCKED"), Slot, X, Y, Facing);
		Build->Cancel();
		return bOk;
	};
	auto PlaceHomeFor = [&](int32 Fam, float X, float Y, int32 Facing)
	{
		if (D)
		{
			D->SelectFamily(Fam);
		}
		if (Place(1, X, Y, Facing) && D)
		{
			D->AssignFamilyToHouse(Fam, Build->GetLastPlaced());
			D->ClearFamilySelection();
		}
	};

	// Acik bolum (16 Eki). Hucre merkezi = -300 + 50i. Ev (4,6) kapi (4,5) | Hastane 2x2 (9,6) kapi (9,5) |
	// AVM 3x3 (5,6) kapi (6,5) | yol (4..10,5) | durak (8,4) yon 1 -> Demir otobusle gelir | Demir evi
	// once (1,10): uzak -> mutsuz | evi (7,4) yon 1'e tasi: kapi (7,5) yolda, duraga 1 -> mutlu -> bitis.
	switch (I)
	{
	case 1: Shot(TEXT("L2_0_baslangic")); break;
	case 2: if (D) { D->SelectFamily(0); } break;
	case 3: Shot(TEXT("L2_1_kaya_istek")); break;
	case 4: PlaceHomeFor(0, -100.f, 0.f, 3); break;
	case 14: Place(2, 175.f, 25.f, 3); break;
	case 15: Place(3, 0.f, 50.f, 3); break;
	case 16: case 17: case 18: case 19: case 20: case 21: case 22:
		Place(4, -300.f + 50.f * (4 + (I - 16)), -50.f, 0);
		break;
	case 23: Shot(TEXT("L2_2_yollar")); break;
	case 24: SLGhost(5, -200.f, -200.f); Shot(TEXT("L2_3a_durak_gecersiz"));
		UE_LOG(LogSLInput, Log, TEXT("SL_DURAK gecersiz bekleniyor: %s (%s)"), *UEnum::GetValueAsString(Build->State), Build->GetBlockedReason()); break;
	case 25: Build->Cancel(); Place(5, 100.f, -100.f, 1); break;
	case 28: Shot(TEXT("L2_3c_demir_geldi")); break;
	case 29: if (D) { D->SelectFamily(1); } break;
	case 30: Shot(TEXT("L2_4_demir_istek")); break;
	case 31: SLGhost(1, -250.f, 200.f); break;
	case 32: Shot(TEXT("L2_5a_uzak_hayalet")); break;
	case 33: if (Build->TryPlace() && D) { D->AssignFamilyToHouse(1, Build->GetLastPlaced()); D->ClearFamilySelection(); } Build->Cancel(); break;
	case 56: Shot(TEXT("L2_5b_demir_mutsuz")); break;
	case 57:
		if (Plat) { Build->PickUp(Plat->FindPlaceableAt(FIntPoint(1, 10))); }
		Build->SetFacing(1);
		Build->SetAimOverride(FVector2D(50.f, -100.f));
		break;
	case 58: Shot(TEXT("L2_6a_tasima_yakin")); break;
	case 59: Build->TryPlace(); Build->Cancel(); break;
	case 63: Shot(TEXT("L2_6b_mutlu")); break;
	case 65: Shot(TEXT("L2_7_tamamlandi")); RotateView(1); break;
	case 68: Shot(TEXT("L2_8_cevrildi")); if (Cam) { Cam->Zoom(5.f); } SLSaat(23.f); break;
	case 70: Shot(TEXT("L2_9_gece_yakin")); break;
	case 72:
		GetWorldTimerManager().ClearTimer(SelfTestTimer);
		UE_LOG(LogSLInput, Log, TEXT("SL_TESTL2_DONE tamam=%d"), GM && GM->IsLevelComplete() ? 1 : 0);
		ConsoleCommand(TEXT("quit"));
		break;
	default: break;
	}
}

void ASLPlayerController::SLTestLevel3()
{
	SelfTestIndex = 0;
	SLSet(TEXT("SwayAmplitude"), TEXT("0"));
	GetWorldTimerManager().SetTimer(SelfTestTimer, this, &ASLPlayerController::TestLevel3Step, 0.5f, true, 1.0f);
}

void ASLPlayerController::TestLevel3Step()
{
	const int32 I = SelfTestIndex++;
	SLReport();
	ASLHUD* HUD = GetHUD<ASLHUD>();
	ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetPawn());
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
	const ASLPlatform* Plat = GM ? GM->GetPlatform() : nullptr;
	if (HUD)
	{
		UE_LOG(LogSLInput, Log, TEXT("SL_HINT %s"), *HUD->GetCurrentHint());
	}
	if (D)
	{
		for (int32 F = 0; F < D->Families.Num(); ++F)
		{
			FString Bubble;
			D->GetFamilyBubble(F, Bubble);
			UE_LOG(LogSLInput, Log, TEXT("SL_FAMILY_STATE %s mutluluk %.2f (%s) balon: %s"), *D->Families[F].Name, D->GetHappiness(F), *D->GetFamilyStatus(F), *Bubble);
		}
		UE_LOG(LogSLInput, Log, TEXT("SL_TIME saat %.1f hamle %d kalan %d"), D->GetTimeHours(), D->GetMoveCount(), D->GetMovesLeft());
	}
	// Gercek fare bu makinede guvenilmez (kullanici oynatiyor): tiklamalarin yaptigini dogrudan cagir.
	// Aileye tiklama = SelectFamily; aile seciliyken ev kurma = AssignFamilyToHouse (OnPlace ayni seyi yapar).
	auto Place = [this](int32 Slot, float X, float Y, int32 Facing)
	{
		SLGhost(Slot, X, Y);
		Build->SetFacing(Facing);
		const bool bOk = Build->TryPlace();
		UE_LOG(LogSLInput, Log, TEXT("SL_PLACE_RESULT %s slot %d (%.0f,%.0f) yon %d"), bOk ? TEXT("OK") : TEXT("BLOCKED"), Slot, X, Y, Facing);
		Build->Cancel();
		return bOk;
	};
	auto PlaceHomeFor = [&](int32 Fam, float X, float Y, int32 Facing)
	{
		if (D)
		{
			D->SelectFamily(Fam);
		}
		if (Place(1, X, Y, Facing) && D)
		{
			D->AssignFamilyToHouse(Fam, Build->GetLastPlaced());
			D->ClearFamilySelection();
		}
	};

	// Acik bolum. Hastane 2x2 (4,6) yon 3 kapi (4,5) | Yilmaz evi (3,5) yon 0 kapi (4,5) | AVM (7,5) kapi
	// (8,4) | Park (2,9) | Demir evi once (6,9): AVM'ye 3, parka 3 -> mutsuz; sonra (4,10) | yol (4,5)
	// (4..8,4) | Durak (6,3) yon 1 -> Aydin gelir | Aydin evi (3,3) yon 0 + yol (4,3). 15 hamle / 24.
	switch (I)
	{
	case 1: Shot(TEXT("L3_0_baslangic")); break;
	case 2: Place(2, -75.f, 25.f, 3); break;                 // Hastane
	case 3: if (D) { D->SelectFamily(0); } break;            // Yilmaz: istekleri acilir
	case 4: Shot(TEXT("L3_1a_yilmaz_istek")); break;
	case 5: SLGhost(1, -150.f, -50.f); Build->SetFacing(0); break;
	case 6: Shot(TEXT("L3_1b_yilmaz_bolge")); break;
	case 7: if (Build->TryPlace() && D) { D->AssignFamilyToHouse(0, Build->GetLastPlaced()); D->ClearFamilySelection(); } Build->Cancel(); break;
	case 20: Place(3, 100.f, 0.f, 3); break;                 // AVM
	case 21: Place(6, -175.f, 175.f, 3); break;              // Park (2,9)
	case 22: if (D) { D->SelectFamily(1); } SLGhost(1, 0.f, 150.f); Build->SetFacing(3); break; // Demir, AVM'ye yakin
	case 23: Shot(TEXT("L3_2a_demir_hayalet_yakin")); break;
	case 24: if (Build->TryPlace() && D) { D->AssignFamilyToHouse(1, Build->GetLastPlaced()); D->ClearFamilySelection(); } Build->Cancel(); break;
	case 40: Shot(TEXT("L3_2b_demir_mutsuz")); break;
	case 41:
		if (Plat) { Build->PickUp(Plat->FindPlaceableAt(FIntPoint(6, 9))); }
		Build->SetAimOverride(FVector2D(-100.f, 200.f));        // (4,10)
		break;
	case 42: Shot(TEXT("L3_2c_tasima")); break;
	case 43: Build->TryPlace(); Build->Cancel(); break;
	case 44: Place(4, -100.f, -50.f, 0); break;              // yol (4,5)
	case 45: case 46: case 47: case 48: case 49:
		Place(4, -300.f + 50.f * (4 + (I - 45)), -100.f, 0);  // yol (4..8, 4)
		break;
	case 50: Shot(TEXT("L3_3_yollar")); break;
	case 51: Place(5, 0.f, -150.f, 1); break;               // Durak (6,3) -> Aydin otobusle gelir
	case 54: Shot(TEXT("L3_4_aydin_geldi")); break;
	case 55: PlaceHomeFor(2, -150.f, -150.f, 0); break;      // Aydin evi (3,3)
	case 56: Place(4, -100.f, -150.f, 0); break;             // yol (4,3)
	case 74: Shot(TEXT("L3_5_tamamlandi")); RotateView(-1); break;
	case 77: Shot(TEXT("L3_6_cevrildi")); SLSaat(21.f); break;
	case 79: Shot(TEXT("L3_7_gece")); break;
	case 81:
		GetWorldTimerManager().ClearTimer(SelfTestTimer);
		UE_LOG(LogSLInput, Log, TEXT("SL_TESTL3_DONE tamam=%d hamle=%d"), GM && GM->IsLevelComplete() ? 1 : 0, D ? D->GetMoveCount() : -1);
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
