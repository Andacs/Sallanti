#include "SLBuildComponent.h"

#include "SLGameMode.h"
#include "SLPlaceable.h"
#include "SLPlatform.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogSLBuild, Log, All);

USLBuildComponent::USLBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Platform once dondurulsun, hayalet ayni karede ona otursun.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GhostMat(TEXT("/Game/Sallanti/Materials/M_Ghost.M_Ghost"));
	GhostMaterial = GhostMat.Object;

	// 1. bolum katalogu. Olculer 08_ASSET_SPEC boyut tablosundan; kutle = hacim (modul^3), ilk tahmin.
	auto Make = [](const TCHAR* Id, const TCHAR* Label, int32 X, int32 Y, int32 Z, float Mass, FLinearColor Tint)
	{
		FSLSlot S;
		S.Id = Id;
		S.Label = Label;
		S.SizeCells = FIntVector(X, Y, Z);
		S.Mass = Mass;
		S.Tint = Tint;
		return S;
	};
	// Ev 1x1 (14 Ekim, Blender taslagi). Kutle 2: hacim kurali 1 verirdi, egitimin ilk adimi hic hissedilmezdi.
	FSLSlot Ev = Make(TEXT("Ev"), TEXT("Ev"), 1, 1, 1, 2.f, FLinearColor(0.80f, 0.55f, 0.35f));
	Ev.Residents = 3;
	Ev.bHasEntrance = true;
	Slots.Add(Ev);
	FSLSlot Hastane = Make(TEXT("Hastane"), TEXT("Hastane"), 2, 2, 3, 12.f, FLinearColor(0.92f, 0.92f, 0.88f)); // orta bina
	Hastane.bHasEntrance = true;
	Slots.Add(Hastane);
	FSLSlot Avm = Make(TEXT("AVM"), TEXT("AVM"), 3, 3, 4, 36.f, FLinearColor(0.40f, 0.30f, 0.22f));             // buyuk bina
	Avm.bHasEntrance = true;
	Slots.Add(Avm);
	FSLSlot Yol = Make(TEXT("Yol"), TEXT("Yol"), 1, 1, 1, 0.5f, FLinearColor(0.25f, 0.22f, 0.20f));
	Yol.bFlat = true;
	Slots.Add(Yol);
}

ASLPlatform* USLBuildComponent::GetPlatform() const
{
	ASLGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASLGameMode>() : nullptr;
	return GM ? GM->GetPlatform() : nullptr;
}

void USLBuildComponent::ConfigurePlaceable(ASLPlaceable* P, const FSLSlot& Slot) const
{
	P->TypeId = Slot.Id;
	P->SizeCells = Slot.SizeCells;
	P->Mass = Slot.Mass;
	P->Tint = Slot.Tint;
	P->bFlat = Slot.bFlat;
	P->Residents = Slot.Residents;
	P->bHasEntrance = Slot.bHasEntrance;
	P->DisplayMesh = Slot.Mesh;
	if (const ASLPlatform* Platform = GetPlatform())
	{
		P->GridSize = Platform->GridSize;
		P->TileGap = Platform->TileGap; // plaka ve yol karoyla hizali olsun
	}
	P->ApplyVisuals();
}

void USLBuildComponent::SetSlotUnlocked(int32 Index, bool bUnlocked)
{
	if (SlotUnlocked.Num() != Slots.Num())
	{
		SlotUnlocked.Init(true, Slots.Num());
	}
	if (SlotUnlocked.IsValidIndex(Index))
	{
		SlotUnlocked[Index] = bUnlocked;
	}
}

void USLBuildComponent::DisableBuilding()
{
	Cancel(); // tasinan nesne varsa eski yerine doner
	bBuildEnabled = false;
}

void USLBuildComponent::SpawnGhostFor(const FSLSlot& Slot)
{
	DestroyGhost();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ghost = GetWorld()->SpawnActor<ASLPlaceable>(ASLPlaceable::StaticClass(), FTransform::Identity, Params);
	if (Ghost)
	{
		ConfigurePlaceable(Ghost, Slot);
		Ghost->MakeGhost(GhostMaterial);
		Ghost->SetEntranceMarkersVisible(true); // yolun nereye gelecegi koymadan once gorunsun
	}
}

void USLBuildComponent::SelectSlot(int32 Index)
{
	if (!bBuildEnabled || !Slots.IsValidIndex(Index) || !IsSlotUnlocked(Index))
	{
		return;
	}
	Cancel(); // tasima yapiliyorduysa nesne yerine doner
	SelectedSlot = Index;
	GhostFacing = Slots[Index].bFlat ? 0 : (DefaultFacing & 3); // yol yonsuz: uzantilar platform ekseninde
	SpawnGhostFor(Slots[Index]);
	State = ESLBuildState::Blocked;
	UpdateGhost();
}

void USLBuildComponent::Cancel()
{
	DestroyGhost();
	if (Moving)
	{
		// Tasima iptal: nesne eski yerine, eski agirligiyla.
		PutAt(Moving, MovingOrigCell, MovingOrigLevel, MovingOrigFacing);
		Moving->SetHeld(false);
		UE_LOG(LogSLBuild, Log, TEXT("SL_MOVE_CANCEL %s eski yerine (%d,%d)"), *Moving->TypeId.ToString(), MovingOrigCell.X, MovingOrigCell.Y);
		Moving = nullptr;
	}
	SelectedSlot = INDEX_NONE;
	State = ESLBuildState::Idle;
}

void USLBuildComponent::RotateGhost(int32 Steps)
{
	if (Ghost)
	{
		SetFacing(GhostFacing + Steps);
	}
}

void USLBuildComponent::SetFacing(int32 InFacing)
{
	GhostFacing = (Ghost && Ghost->bFlat) ? 0 : ((InFacing % 4) + 4) % 4;
	UpdateGhost();
}

void USLBuildComponent::ShowEntranceMarkers(bool bShow) const
{
	if (const ASLPlatform* Platform = GetPlatform())
	{
		for (const TWeakObjectPtr<ASLPlaceable>& Weak : Platform->GetPlaceables())
		{
			if (ASLPlaceable* P = Weak.Get())
			{
				P->SetEntranceMarkersVisible(bShow && !P->bHeld);
			}
		}
	}
}

void USLBuildComponent::DestroyGhost()
{
	if (Ghost)
	{
		Ghost->Destroy();
		Ghost = nullptr;
	}
}

void USLBuildComponent::SetAimOverride(const FVector2D& LocalXY)
{
	bAimOverride = true;
	AimOverride = LocalXY;
	UpdateGhost();
}

bool USLBuildComponent::GetAimLocal(FVector2D& OutLocalXY) const
{
	const ASLPlatform* Platform = GetPlatform();
	if (!Platform)
	{
		return false;
	}
	if (bAimOverride)
	{
		OutLocalXY = AimOverride;
		return true;
	}
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return false;
	}
	const FTransform PivotXf = Platform->TiltPivot->GetComponentTransform();

	// Once fiziksel isin: platforma ya da uzerindeki bir nesneye degiyorsa o nokta.
	// Normal yonunde 1 cm disari: ust yuzde ayni hucre, yan yuzde komsu hucre.
	FHitResult Hit;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.GetActor())
	{
		const AActor* HitActor = Hit.GetActor();
		if (HitActor == Platform || HitActor->GetAttachParentActor() == Platform)
		{
			const FVector Local = PivotXf.InverseTransformPosition(Hit.ImpactPoint + Hit.ImpactNormal * 1.f);
			OutLocalXY = FVector2D(Local.X, Local.Y);
			return true;
		}
	}

	// Degmiyorsa platform duzlemiyle kesisim: hayalet disarida kirmizi gorunsun.
	FVector Origin, Dir;
	if (!PC->DeprojectMousePositionToWorld(Origin, Dir))
	{
		return false;
	}
	const FPlane DeckPlane(PivotXf.GetLocation(), PivotXf.GetUnitAxis(EAxis::Z));
	const float Denom = FVector::DotProduct(Dir, DeckPlane.GetNormal());
	if (FMath::Abs(Denom) < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const FVector OnPlane = FMath::RayPlaneIntersection(Origin, Dir, DeckPlane);
	const FVector Local = PivotXf.InverseTransformPosition(OnPlane);
	OutLocalXY = FVector2D(Local.X, Local.Y);
	return true;
}

void USLBuildComponent::UpdateGhost()
{
	ASLPlatform* Platform = GetPlatform();
	if (!Ghost || !Platform)
	{
		return;
	}

	FVector2D Aim;
	if (!GetAimLocal(Aim))
	{
		Ghost->SetActorHiddenInGame(true);
		State = ESLBuildState::Blocked;
		BlockedReason = TEXT("imleç platformda değil");
		return;
	}
	Ghost->SetActorHiddenInGame(false);
	Ghost->Facing = GhostFacing;

	// Izgaraya yapisma: ayak izinin merkezi imlece en yakin hucre kosesine oturur.
	const float G = Platform->GridSize;
	const FIntPoint Foot = Ghost->GetFootprint();
	const int32 N = Platform->GetCellCount();
	const FIntPoint CellMin(
		FMath::RoundToInt((Aim.X + Platform->HalfExtent) / G - 0.5f * Foot.X),
		FMath::RoundToInt((Aim.Y + Platform->HalfExtent) / G - 0.5f * Foot.Y));

	const bool bInside = CellMin.X >= 0 && CellMin.Y >= 0 && CellMin.X + Foot.X <= N && CellMin.Y + Foot.Y <= N;
	bool bValid = bInside;
	BlockedReason = bInside ? TEXT("") : TEXT("kenardan taşıyor ya da dışarıda");
	GhostCellMin = CellMin;
	GhostLevel = 0;
	Ghost->CellMin = CellMin; // kapi onu hucreleri (ipucu) hayaletten okunsun

	if (bInside)
	{
		if (Platform->bAllowStacking && !Ghost->bFlat)
		{
			// Istif: ayak izinin altindaki en yuksek nesnenin ustune oturur; duz nesnenin ustune degil.
			GhostLevel = Platform->GetStackLevel(CellMin, Foot);
		}
		else if (!Platform->IsAreaFree(CellMin, Foot, Moving))
		{
			bValid = false; // sehir plani: her hucrede tek nesne
			BlockedReason = TEXT("hücre dolu");
		}
	}

	const FVector LocalCenter(
		Platform->CellToLocal(CellMin.X + 0.5f * Foot.X),
		Platform->CellToLocal(CellMin.Y + 0.5f * Foot.Y),
		GhostLevel * G);
	GhostCenterLocal = FVector2D(LocalCenter.X, LocalCenter.Y);
	const FQuat LocalRot(FVector::UpVector, GhostFacing * UE_HALF_PI);
	const FTransform LocalXf(LocalRot, LocalCenter);
	Ghost->SetActorTransform(LocalXf * Platform->TiltPivot->GetComponentTransform());

	State = bValid ? ESLBuildState::Placing : ESLBuildState::Blocked;
	FLinearColor Color = bValid ? ValidColor : BlockedColor;
	if (bValid)
	{
		const float Pred = Platform->PredictTiltDegrees(Ghost->Mass, GhostCenterLocal);
		Color = Pred >= Platform->FailTilt ? DangerColor : Pred >= Platform->WarnTilt ? RiskyColor : ValidColor;
	}
	Ghost->SetGhostColor(Color, GhostOpacity);
}

void USLBuildComponent::PutAt(ASLPlaceable* P, FIntPoint Cell, int32 Level, int32 InFacing) const
{
	const ASLPlatform* Platform = GetPlatform();
	if (!P || !Platform)
	{
		return;
	}
	P->CellMin = Cell;
	P->Level = Level;
	P->Facing = InFacing & 3;
	const FIntPoint Foot = P->GetFootprint();
	// Dunya donusumunden geri hesaplanan yerel konum yuvarlama tasiyabilir: tam degeri yaz.
	P->SetActorRelativeLocation(FVector(
		Platform->CellToLocal(Cell.X + 0.5f * Foot.X),
		Platform->CellToLocal(Cell.Y + 0.5f * Foot.Y),
		Level * Platform->GridSize));
	P->SetActorRelativeRotation(FRotator(0.f, 90.f * P->Facing, 0.f));
}

bool USLBuildComponent::TryPlace()
{
	ASLPlatform* Platform = GetPlatform();
	if (!bBuildEnabled || !Ghost || !Platform)
	{
		return false;
	}
	UpdateGhost();
	if (State != ESLBuildState::Placing)
	{
		return false; // gecersiz: ses/uyari yok, kirmizi hayalet yeterli
	}

	if (Moving)
	{
		// Tasima: ayni nesne yeni yerinde; yeni dogma yok, aile ve gorev durumu korunur.
		ASLPlaceable* P = Moving;
		Moving = nullptr;
		PutAt(P, GhostCellMin, GhostLevel, GhostFacing);
		P->SetHeld(false);
		DestroyGhost();
		State = ESLBuildState::Idle;
		UE_LOG(LogSLBuild, Log, TEXT("SL_MOVE %s (%d,%d) yon %d -> (%d,%d) yon %d"), *P->TypeId.ToString(),
			MovingOrigCell.X, MovingOrigCell.Y, MovingOrigFacing, GhostCellMin.X, GhostCellMin.Y, GhostFacing);
		return true;
	}

	if (!Slots.IsValidIndex(SelectedSlot) || !IsSlotUnlocked(SelectedSlot))
	{
		return false;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASLPlaceable* P = GetWorld()->SpawnActor<ASLPlaceable>(ASLPlaceable::StaticClass(), Ghost->GetActorTransform(), Params);
	if (!P)
	{
		return false;
	}
	ConfigurePlaceable(P, Slots[SelectedSlot]);
	P->AttachToComponent(Platform->TiltPivot, FAttachmentTransformRules::KeepWorldTransform);
	PutAt(P, GhostCellMin, GhostLevel, GhostFacing);

	Platform->RegisterPlaceable(P);
	PlacedStack.Add(P);
	++PlacedCount;
	FString Ent;
	for (const FIntPoint& E : P->GetEntranceCells())
	{
		Ent += FString::Printf(TEXT("(%d,%d)"), E.X, E.Y);
	}
	UE_LOG(LogSLBuild, Log, TEXT("SL_PLACE %s cell=(%d,%d) level=%d foot=(%d,%d) yon=%d giris=%s mass=%.1f rel=%s"),
		*P->TypeId.ToString(), GhostCellMin.X, GhostCellMin.Y, GhostLevel, P->GetFootprint().X, P->GetFootprint().Y,
		P->Facing, Ent.IsEmpty() ? TEXT("-") : *Ent, P->Mass, *P->GetRootComponent()->GetRelativeLocation().ToString());
	UpdateGhost();
	return true;
}

bool USLBuildComponent::PickUp(ASLPlaceable* Target)
{
	ASLPlatform* Platform = GetPlatform();
	if (!bBuildEnabled || !Target || !Platform || Target->GetAttachParentActor() != Platform)
	{
		return false;
	}
	if (Platform->bAllowStacking && Platform->GetStackLevel(Target->CellMin, Target->GetFootprint()) > Target->Level + Target->GetStackHeight())
	{
		return false; // ustunde baska nesne var
	}
	Cancel();
	Moving = Target;
	MovingOrigCell = Target->CellMin;
	MovingOrigLevel = Target->Level;
	MovingOrigFacing = Target->Facing;
	GhostFacing = Target->Facing;
	Target->SetHeld(true); // agirligi bu kareden itibaren dengeden cikar

	const int32 SlotIdx = FindSlotById(Target->TypeId);
	FSLSlot Shape = Slots.IsValidIndex(SlotIdx) ? Slots[SlotIdx] : FSLSlot();
	Shape.SizeCells = Target->SizeCells;
	Shape.bFlat = Target->bFlat;
	Shape.bHasEntrance = Target->bHasEntrance;
	SpawnGhostFor(Shape);
	State = ESLBuildState::Blocked;
	UE_LOG(LogSLBuild, Log, TEXT("SL_PICKUP %s (%d,%d)"), *Target->TypeId.ToString(), MovingOrigCell.X, MovingOrigCell.Y);
	UpdateGhost();
	return true;
}

bool USLBuildComponent::TryPickUpUnderCursor()
{
	const ASLPlatform* Platform = GetPlatform();
	if (!Platform || Ghost)
	{
		return false;
	}
	// Once dogrudan isin: tiklanan nesne. Olmazsa imlecin dustugu hucredeki nesne (yol gibi ince seyler).
	if (const APlayerController* PC = Cast<APlayerController>(GetOwner()); PC && !bAimOverride)
	{
		FHitResult Hit;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			if (ASLPlaceable* P = Cast<ASLPlaceable>(Hit.GetActor()))
			{
				return PickUp(P);
			}
		}
	}
	FVector2D Aim;
	if (GetAimLocal(Aim))
	{
		return PickUp(Platform->FindPlaceableAt(Platform->LocalToCell(Aim)));
	}
	return false;
}

void USLBuildComponent::UndoLast()
{
	if (!bBuildEnabled)
	{
		return; // devrildikten sonra geri alma yok: F5 / Space ile bastan
	}
	Cancel();
	while (PlacedStack.Num() > 0)
	{
		ASLPlaceable* P = PlacedStack.Pop().Get();
		if (P)
		{
			// Platform zayif isaretci tutuyor; yok olan nesne bir sonraki karede dengeden duser.
			P->Destroy();
			PlacedCount = FMath::Max(0, PlacedCount - 1);
			UE_LOG(LogSLBuild, Log, TEXT("SL_UNDO count=%d"), PlacedCount);
			break;
		}
	}
}

void USLBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateGhost();
	// Yerlestirirken her binanin kapi onu gorunsun: yol nereye gelmeli, kapinin onu kapanmasin.
	const bool bPlacing = State != ESLBuildState::Idle;
	if (bPlacing || bPlacing != bMarkersShown)
	{
		ShowEntranceMarkers(bPlacing); // yerlestirirken her kare: yeni konan binalar da
		bMarkersShown = bPlacing;
	}
}
