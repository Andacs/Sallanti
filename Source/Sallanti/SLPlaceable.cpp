#include "SLPlaceable.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

ASLPlaceable::ASLPlaceable()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetSimulatePhysics(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CardboardMat(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	DefaultCube = CubeMesh.Object;
	if (CardboardMat.Succeeded())
	{
		Mesh->SetMaterial(0, CardboardMat.Object);
	}
}

void ASLPlaceable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisuals();
}

void ASLPlaceable::ApplyVisuals()
{
	const FVector SizeCm(SizeCells.X * GridSize, SizeCells.Y * GridSize, SizeCells.Z * GridSize);
	if (DisplayMesh)
	{
		Mesh->SetStaticMesh(DisplayMesh);
		Mesh->SetRelativeScale3D(FVector::OneVector);
		Mesh->SetRelativeLocation(FVector::ZeroVector);
	}
	else
	{
		// Motor kupu 100 cm, merkez pivotlu: boyuta olcekle, tabani Z=0'a kaldir.
		Mesh->SetStaticMesh(DefaultCube);
		Mesh->SetRelativeScale3D(SizeCm / 100.f);
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, 0.5f * SizeCm.Z));
	}

	if (!bGhost)
	{
		if (!MID)
		{
			MID = Mesh->CreateDynamicMaterialInstance(0);
		}
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Tint"), Tint);
		}
	}
}

void ASLPlaceable::Release(UPhysicalMaterial* PhysMat, float KgPerMass)
{
	if (bReleased || bGhost)
	{
		return;
	}
	bReleased = true;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	// BlockAll = WorldStatic; simule edilen govde PhysicsActor olmali.
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	if (PhysMat)
	{
		Mesh->SetPhysMaterialOverride(PhysMat);
	}
	Mesh->SetMassOverrideInKg(NAME_None, FMath::Max(0.1f, Mass * KgPerMass), true);
	Mesh->SetSimulatePhysics(true);
	Mesh->WakeAllRigidBodies();
}

FVector ASLPlaceable::GetBodyCenter() const
{
	// Simule edilen mesh aktor kokunden ayrilir; gercek yer mesh'in sinir kutusunun merkezi.
	return Mesh->Bounds.Origin;
}

void ASLPlaceable::MakeGhost(UMaterialInterface* GhostMaterial)
{
	bGhost = true;
	SetActorEnableCollision(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	if (GhostMaterial)
	{
		Mesh->SetMaterial(0, GhostMaterial);
		MID = Mesh->CreateDynamicMaterialInstance(0, GhostMaterial);
	}
}

void ASLPlaceable::SetGhostColor(const FLinearColor& Color, float Opacity)
{
	if (bGhost && MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
}
