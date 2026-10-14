#include "SLPlatform.h"

#include "SLPlaceable.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASLPlatform::ASLPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	TiltPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TiltPivot"));
	TiltPivot->SetupAttachment(Root);
	TiltPivot->SetMobility(EComponentMobility::Movable);

	Deck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Deck"));
	Deck->SetupAttachment(TiltPivot);
	Deck->SetMobility(EComponentMobility::Movable);
	Deck->SetCollisionProfileName(TEXT("BlockAll"));

	// Python'dan C++ CDO'ya yazilan referans kalici olmuyor (04_UNREAL_TUZAKLARI): constructor'da bagla.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CardboardMat(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	if (CubeMesh.Succeeded())
	{
		Deck->SetStaticMesh(CubeMesh.Object);
	}
	if (CardboardMat.Succeeded())
	{
		Deck->SetMaterial(0, CardboardMat.Object);
	}

	GridLines = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridLines"));
	GridLines->SetupAttachment(TiltPivot);
	GridLines->SetMobility(EComponentMobility::Movable);
	GridLines->SetCollisionEnabled(ECollisionEnabled::NoCollision); // isini engellemesin
	GridLines->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		GridLines->SetStaticMesh(CubeMesh.Object);
	}
	if (CardboardMat.Succeeded())
	{
		GridLines->SetMaterial(0, CardboardMat.Object);
	}
	ApplyDeckSize();
}

void ASLPlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDeckSize();
	BuildGrid();
}

void ASLPlatform::BuildGrid()
{
	GridLines->ClearInstances();
	const int32 N = GetCellCount();
	const float Len = 2.f * HalfExtent;
	const float W = GridLineWidth / 100.f;
	const float H = 0.5f / 100.f; // 0.5 cm yukseklik, yuzeyin 0.25 cm ustunde: z-fighting yok
	for (int32 I = 0; I <= N; ++I)
	{
		const float P = CellToLocal(I);
		GridLines->AddInstance(FTransform(FQuat::Identity, FVector(P, 0.f, 0.25f), FVector(W, Len / 100.f, H)));
		GridLines->AddInstance(FTransform(FQuat::Identity, FVector(0.f, P, 0.25f), FVector(Len / 100.f, W, H)));
	}
	if (UMaterialInstanceDynamic* MID = GridLines->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Tint"), GridColor);
	}
	GridLines->SetVisibility(bGridAlwaysVisible || bGridForPlacing);
}

void ASLPlatform::SetGridShownForPlacing(bool bPlacing)
{
	bGridForPlacing = bPlacing;
	GridLines->SetVisibility(bGridAlwaysVisible || bGridForPlacing);
}

void ASLPlatform::ApplyDeckSize()
{
	// Motor kupu 100 cm ve merkez pivotlu: ust yuzeyi yerel Z=0'a indir.
	Deck->SetRelativeScale3D(FVector(2.f * HalfExtent / 100.f, 2.f * HalfExtent / 100.f, DeckThickness / 100.f));
	Deck->SetRelativeLocation(FVector(0.f, 0.f, -0.5f * DeckThickness));
}

void ASLPlatform::RegisterPlaceable(ASLPlaceable* Placeable)
{
	if (Placeable)
	{
		Placeables.Add(Placeable);
		RecomputeCenterOfMass();
	}
}

void ASLPlatform::RecomputeCenterOfMass()
{
	FVector2D Weighted = FVector2D::ZeroVector;
	float Mass = 0.f;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		const ASLPlaceable* P = Weak.Get();
		if (!P || P->bHeld)
		{
			continue; // geri alinmis ya da oyuncunun elinde: dengeye katilmaz
		}
		const FVector Rel = P->GetRootComponent()->GetRelativeLocation();
		Weighted += P->Mass * FVector2D(Rel.X, Rel.Y);
		Mass += P->Mass;
	}
	TotalMass = Mass;
	const float Denominator = Mass + PlatformMass;
	CenterOfMass = Denominator > KINDA_SMALL_NUMBER ? Weighted / Denominator : FVector2D::ZeroVector;
}

int32 ASLPlatform::GetStackLevel(FIntPoint CellMin, FIntPoint Footprint) const
{
	int32 Level = 0;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		const ASLPlaceable* P = Weak.Get();
		if (!P || P->bHeld || P->GetAttachParentActor() != this)
		{
			continue;
		}
		const FIntPoint OMin = P->CellMin;
		const FIntPoint OSize = P->GetFootprint();
		const bool bOverlapX = CellMin.X < OMin.X + OSize.X && OMin.X < CellMin.X + Footprint.X;
		const bool bOverlapY = CellMin.Y < OMin.Y + OSize.Y && OMin.Y < CellMin.Y + Footprint.Y;
		if (bOverlapX && bOverlapY)
		{
			Level = FMath::Max(Level, P->Level + P->GetStackHeight());
		}
	}
	return Level;
}

bool ASLPlatform::IsAreaFree(FIntPoint CellMin, FIntPoint Footprint, const ASLPlaceable* Ignore) const
{
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		const ASLPlaceable* P = Weak.Get();
		if (!P || P == Ignore || P->bHeld)
		{
			continue;
		}
		const FIntPoint OMin = P->CellMin;
		const FIntPoint OSize = P->GetFootprint();
		if (CellMin.X < OMin.X + OSize.X && OMin.X < CellMin.X + Footprint.X
			&& CellMin.Y < OMin.Y + OSize.Y && OMin.Y < CellMin.Y + Footprint.Y)
		{
			return false;
		}
	}
	return true;
}

ASLPlaceable* ASLPlatform::FindPlaceableAt(const FIntPoint& Cell) const
{
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		ASLPlaceable* P = Weak.Get();
		if (P && !P->bHeld && P->CoversCell(Cell))
		{
			return P;
		}
	}
	return nullptr;
}

float ASLPlatform::GetWorldZOfLocal(const FVector& Local) const
{
	return TiltPivot->GetComponentTransform().TransformPosition(Local).Z;
}

void ASLPlatform::BeginPlay()
{
	Super::BeginPlay();
	DeckMID = Deck->CreateDynamicMaterialInstance(0);
	if (DeckMID)
	{
		DeckMID->SetVectorParameterValue(TEXT("Tint"), DeckTint);
	}
}

void ASLPlatform::ApplyTiltRotation(const FVector2D& TiltDeg)
{
	// d = inen taraf. k = Z x d ekseni etrafinda pozitif aci d tarafini asagi indirir (SLReport ile olculdu).
	const float Deg = TiltDeg.Size();
	FQuat Rot = FQuat::Identity;
	if (Deg > KINDA_SMALL_NUMBER)
	{
		const FVector Down(TiltDeg.X / Deg, TiltDeg.Y / Deg, 0.f);
		const FVector Axis = FVector::CrossProduct(FVector::UpVector, Down).GetSafeNormal();
		Rot = FQuat(Axis, FMath::DegreesToRadians(Deg));
	}
	TiltPivot->SetRelativeRotation(Rot);
}

void ASLPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (BalanceState == ESLBalanceState::Collapsed)
	{
		// Devrilme: ayni yone, hizlanarak, CollapseAngle'a kadar. Binalar bagli, birlikte gider.
		CollapseVelocity += CollapseAcceleration * DeltaSeconds;
		CollapseCurrent = FMath::Min(CollapseCurrent + CollapseVelocity * DeltaSeconds, CollapseAngle);
		CurrentTiltDegrees = CollapseCurrent;
		ApplyTiltRotation(CollapseDir * CollapseCurrent);
		UpdateDeckWarning(DeltaSeconds);
		return;
	}

	RecomputeCenterOfMass();

	// Hedef egim: CoM'un oldugu taraf iner. Boy = |CoM| * TiltPerCm, MaxTilt ile kirpilir.
	const float ComDist = CenterOfMass.Size();
	const FVector2D TargetTilt = ComDist > KINDA_SMALL_NUMBER
		? (CenterOfMass / ComDist) * FMath::Min(ComDist * TiltPerCm, MaxTilt)
		: FVector2D::ZeroVector;
	TargetTiltDegrees = TargetTilt.Size();
	CurrentTilt = FMath::Vector2DInterpConstantTo(CurrentTilt, TargetTilt, DeltaSeconds, TiltSpeed);

	SwayTime += DeltaSeconds;
	const float SwayRad = FMath::DegreesToRadians(SwayDirection);
	const FVector2D SwayDir(FMath::Cos(SwayRad), FMath::Sin(SwayRad));
	const FVector2D Sway = SwayDir * (FMath::Sin(SwayTime * UE_TWO_PI / SwayPeriod) * SwayAmplitude);

	const FVector2D Total = CurrentTilt + Sway;
	CurrentTiltDegrees = Total.Size();
	ApplyTiltRotation(Total);

	UpdateBalanceState(DeltaSeconds);
	UpdateDeckWarning(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	if (bDebugDraw)
	{
		const FVector ComWorld = TiltPivot->GetComponentTransform().TransformPosition(FVector(CenterOfMass.X, CenterOfMass.Y, 2.f));
		// Sayilar ASLHUD test panelinde; burada sadece CoM'un platformdaki yeri.
		DrawDebugSphere(GetWorld(), ComWorld, 12.f, 8, FColor::Magenta, false, -1.f, SDPG_Foreground);
	}
#endif
}

void ASLPlatform::UpdateBalanceState(float DeltaSeconds)
{
	const float Judged = CurrentTilt.Size(); // sallanma haric
	WarnAlpha = FMath::Clamp((Judged - WarnTilt) / FMath::Max(FailTilt - WarnTilt, 0.01f), 0.f, 1.f);

	if (Judged >= FailTilt)
	{
		FailTimer += DeltaSeconds;
		BalanceState = ESLBalanceState::Failing;
		if (FailTimer >= FailSeconds)
		{
			Collapse();
		}
		return;
	}
	FailTimer = 0.f; // kesintisiz olmali: esigin altina inen sayaci sifirlar
	BalanceState = Judged >= WarnTilt ? ESLBalanceState::Warning : ESLBalanceState::Stable;
}

void ASLPlatform::UpdateDeckWarning(float DeltaSeconds)
{
	if (!DeckMID)
	{
		return;
	}
	float Strength = 0.f;
	float Hz = 0.f;
	switch (BalanceState)
	{
	case ESLBalanceState::Warning:
		Strength = FMath::Lerp(WarnStrengthMin, WarnStrengthMax, WarnAlpha);
		Hz = FMath::Lerp(WarnPulseHzMin, WarnPulseHzMax, WarnAlpha);
		break;
	case ESLBalanceState::Failing:   Strength = WarnStrengthMax; Hz = FailingPulseHz; break;
	case ESLBalanceState::Collapsed: Strength = WarnStrengthMax; Hz = 0.f; break;
	default: break;
	}
	WarnPhase += DeltaSeconds * Hz * UE_TWO_PI;
	// Nabiz yarim genlikte: en sonuk aninda bile Strength'in yarisi kadar kirmizi kalir, uyari kaybolmaz.
	const float Pulse = Hz > 0.f ? 0.75f + 0.25f * FMath::Sin(WarnPhase) : 1.f;
	DeckMID->SetVectorParameterValue(TEXT("Tint"), FMath::Lerp(DeckTint, WarnColor, Strength * Pulse));
}

void ASLPlatform::Collapse()
{
	BalanceState = ESLBalanceState::Collapsed;
	const float Deg = CurrentTilt.Size();
	CollapseDir = Deg > KINDA_SMALL_NUMBER ? CurrentTilt / Deg : FVector2D(1.f, 0.f);
	CollapseCurrent = Deg;
	CollapseVelocity = TiltSpeed * 0.5f; // eldeki egimden yumusak devam
	UE_LOG(LogTemp, Log, TEXT("SL_COLLAPSE egim %.2f deg, %.2f sn FailTilt (%.1f) ustunde, yon (%.2f, %.2f)"),
		Deg, FailTimer, FailTilt, CollapseDir.X, CollapseDir.Y);
	OnCollapsed.Broadcast();
}
