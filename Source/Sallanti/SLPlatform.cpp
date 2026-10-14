#include "SLPlatform.h"

#include "SLPlaceable.h"
#include "Components/StaticMeshComponent.h"
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
	ApplyDeckSize();
}

void ASLPlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDeckSize();
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
		if (!P || P->GetAttachParentActor() != this)
		{
			continue; // fizige birakilmis ya da yok olmus: dengeye katilmaz
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
		if (!P || P->GetAttachParentActor() != this)
		{
			continue;
		}
		const FIntPoint OMin = P->CellMin;
		const FIntPoint OSize = P->GetFootprint();
		const bool bOverlapX = CellMin.X < OMin.X + OSize.X && OMin.X < CellMin.X + Footprint.X;
		const bool bOverlapY = CellMin.Y < OMin.Y + OSize.Y && OMin.Y < CellMin.Y + Footprint.Y;
		if (bOverlapX && bOverlapY)
		{
			Level = FMath::Max(Level, P->Level + P->SizeCells.Z);
		}
	}
	return Level;
}

float ASLPlatform::GetWorldZOfLocal(const FVector& Local) const
{
	return TiltPivot->GetComponentTransform().TransformPosition(Local).Z;
}

void ASLPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RecomputeCenterOfMass();

	// Hedef egim: CoM'un oldugu taraf iner. Boy = |CoM| * TiltPerCm, MaxTilt ile kirpilir.
	const float ComDist = CenterOfMass.Size();
	const FVector2D TargetTilt = ComDist > KINDA_SMALL_NUMBER
		? (CenterOfMass / ComDist) * FMath::Min(ComDist * TiltPerCm, MaxTilt)
		: FVector2D::ZeroVector;
	CurrentTilt = FMath::Vector2DInterpConstantTo(CurrentTilt, TargetTilt, DeltaSeconds, TiltSpeed);

	SwayTime += DeltaSeconds;
	const float SwayRad = FMath::DegreesToRadians(SwayDirection);
	const FVector2D SwayDir(FMath::Cos(SwayRad), FMath::Sin(SwayRad));
	const FVector2D Sway = SwayDir * (FMath::Sin(SwayTime * UE_TWO_PI / SwayPeriod) * SwayAmplitude);

	const FVector2D Total = CurrentTilt + Sway;
	CurrentTiltDegrees = Total.Size();

	// d = inen taraf. k = Z x d ekseni etrafinda pozitif aci d tarafini asagi indirir (SLReport ile olculdu).
	FQuat Rot = FQuat::Identity;
	if (CurrentTiltDegrees > KINDA_SMALL_NUMBER)
	{
		const FVector Down(Total.X / CurrentTiltDegrees, Total.Y / CurrentTiltDegrees, 0.f);
		const FVector Axis = FVector::CrossProduct(FVector::UpVector, Down).GetSafeNormal();
		Rot = FQuat(Axis, FMath::DegreesToRadians(CurrentTiltDegrees));
	}
	TiltPivot->SetRelativeRotation(Rot);

#if !UE_BUILD_SHIPPING
	if (bDebugDraw)
	{
		const FVector ComWorld = TiltPivot->GetComponentTransform().TransformPosition(FVector(CenterOfMass.X, CenterOfMass.Y, 2.f));
		DrawDebugSphere(GetWorld(), ComWorld, 12.f, 8, FColor::Magenta, false, -1.f, SDPG_Foreground);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9001, 0.f, FColor::Yellow, FString::Printf(
				TEXT("CoM (%.0f, %.0f) cm | kutle %.1f | hedef %.2f deg | egim %.2f deg (sallanma dahil %.2f)"),
				CenterOfMass.X, CenterOfMass.Y, TotalMass, TargetTilt.Size(), CurrentTilt.Size(), CurrentTiltDegrees));
		}
	}
#endif
}
