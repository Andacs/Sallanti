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

	// 08_ASSET_SPEC boyut tablosu. Kutle = hacim (modul^3); ilk tahmin, oynayarak ayarlanacak.
	auto Make = [](int32 X, int32 Y, int32 Z, float Mass, FLinearColor Tint)
	{
		FSLSlot S;
		S.SizeCells = FIntVector(X, Y, Z);
		S.Mass = Mass;
		S.Tint = Tint;
		return S;
	};
	Slots.Add(Make(1, 1, 1, 1.f, FLinearColor(0.62f, 0.45f, 0.28f)));  // kutu
	Slots.Add(Make(1, 1, 2, 2.f, FLinearColor(0.80f, 0.70f, 0.52f)));  // orta nesne
	Slots.Add(Make(2, 2, 2, 8.f, FLinearColor(0.85f, 0.85f, 0.80f)));  // kucuk bina
	Slots.Add(Make(2, 2, 3, 12.f, FLinearColor(0.38f, 0.27f, 0.18f))); // orta bina
}

ASLPlatform* USLBuildComponent::GetPlatform() const
{
	ASLGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASLGameMode>() : nullptr;
	return GM ? GM->GetPlatform() : nullptr;
}

void USLBuildComponent::ConfigurePlaceable(ASLPlaceable* P, const FSLSlot& Slot) const
{
	P->SizeCells = Slot.SizeCells;
	P->Mass = Slot.Mass;
	P->Tint = Slot.Tint;
	P->DisplayMesh = Slot.Mesh;
	if (const ASLPlatform* Platform = GetPlatform())
	{
		P->GridSize = Platform->GridSize;
	}
	P->ApplyVisuals();
}

void USLBuildComponent::SelectSlot(int32 Index)
{
	if (!Slots.IsValidIndex(Index))
	{
		return;
	}
	DestroyGhost();
	SelectedSlot = Index;
	bGhostRotated = false;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ghost = GetWorld()->SpawnActor<ASLPlaceable>(ASLPlaceable::StaticClass(), FTransform::Identity, Params);
	if (Ghost)
	{
		ConfigurePlaceable(Ghost, Slots[Index]);
		Ghost->MakeGhost(GhostMaterial);
	}
	State = ESLBuildState::Blocked;
	UpdateGhost();
}

void USLBuildComponent::Cancel()
{
	DestroyGhost();
	SelectedSlot = INDEX_NONE;
	State = ESLBuildState::Idle;
}

void USLBuildComponent::RotateGhost()
{
	if (Ghost)
	{
		bGhostRotated = !bGhostRotated;
		UpdateGhost();
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
		return;
	}
	Ghost->SetActorHiddenInGame(false);
	Ghost->bRotated = bGhostRotated;

	// Izgaraya yapisma: ayak izinin merkezi imlece en yakin hucre kosesine oturur.
	const float G = Platform->GridSize;
	const FIntPoint Foot = Ghost->GetFootprint();
	const int32 N = Platform->GetCellCount();
	const FIntPoint CellMin(
		FMath::RoundToInt((Aim.X + Platform->HalfExtent) / G - 0.5f * Foot.X),
		FMath::RoundToInt((Aim.Y + Platform->HalfExtent) / G - 0.5f * Foot.Y));

	// Gecersiz: taban platform kenarindan tasiyor (isin disarida ise de buraya duser).
	const bool bInside = CellMin.X >= 0 && CellMin.Y >= 0 && CellMin.X + Foot.X <= N && CellMin.Y + Foot.Y <= N;

	// Ayak izinin altindaki en yuksek nesnenin ustune oturur; boylece cakisma olusamaz.
	GhostCellMin = CellMin;
	GhostLevel = bInside ? Platform->GetStackLevel(CellMin, Foot) : 0;

	const FVector LocalCenter(
		Platform->CellToLocal(CellMin.X + 0.5f * Foot.X),
		Platform->CellToLocal(CellMin.Y + 0.5f * Foot.Y),
		GhostLevel * G);
	const FQuat LocalRot = bGhostRotated ? FQuat(FVector::UpVector, UE_HALF_PI) : FQuat::Identity;
	const FTransform LocalXf(LocalRot, LocalCenter);
	Ghost->SetActorTransform(LocalXf * Platform->TiltPivot->GetComponentTransform());

	State = bInside ? ESLBuildState::Placing : ESLBuildState::Blocked;
	Ghost->SetGhostColor(bInside ? ValidColor : BlockedColor, GhostOpacity);
}

bool USLBuildComponent::TryPlace()
{
	ASLPlatform* Platform = GetPlatform();
	if (!Ghost || !Platform || !Slots.IsValidIndex(SelectedSlot))
	{
		return false;
	}
	UpdateGhost();
	if (State != ESLBuildState::Placing)
	{
		return false; // gecersiz: ses/uyari yok, kirmizi hayalet yeterli
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASLPlaceable* P = GetWorld()->SpawnActor<ASLPlaceable>(ASLPlaceable::StaticClass(), Ghost->GetActorTransform(), Params);
	if (!P)
	{
		return false;
	}
	ConfigurePlaceable(P, Slots[SelectedSlot]);
	P->CellMin = GhostCellMin;
	P->Level = GhostLevel;
	P->bRotated = bGhostRotated;
	P->AttachToComponent(Platform->TiltPivot, FAttachmentTransformRules::KeepWorldTransform);

	// Dunya donusumunden geri hesaplanan yerel konum yuvarlama tasiyabilir: tam degeri yaz.
	const FIntPoint Foot = P->GetFootprint();
	const float G = Platform->GridSize;
	P->SetActorRelativeLocation(FVector(
		Platform->CellToLocal(GhostCellMin.X + 0.5f * Foot.X),
		Platform->CellToLocal(GhostCellMin.Y + 0.5f * Foot.Y),
		GhostLevel * G));
	P->SetActorRelativeRotation(bGhostRotated ? FRotator(0.f, 90.f, 0.f) : FRotator::ZeroRotator);

	Platform->RegisterPlaceable(P);
	++PlacedCount;
	UE_LOG(LogSLBuild, Log, TEXT("SL_PLACE slot=%d cell=(%d,%d) level=%d foot=(%d,%d) mass=%.1f rel=%s"),
		SelectedSlot, GhostCellMin.X, GhostCellMin.Y, GhostLevel, Foot.X, Foot.Y, P->Mass,
		*P->GetRootComponent()->GetRelativeLocation().ToString());
	UpdateGhost();
	return true;
}

void USLBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateGhost();
}
