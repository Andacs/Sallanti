#include "SLFamily.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASLFamily::ASLFamily()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CardboardMat(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	Cube = CubeMesh.Object;
	BaseMat = CardboardMat.Object;
	SetActorEnableCollision(false); // imlec isinini engellemesin
}

void ASLFamily::BuildFigures()
{
	if (!Cube || Figures.Num() > 0)
	{
		return;
	}
	const float W = 30.f;
	for (int32 I = 0; I < Members; ++I)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Cube);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(GetRootComponent());
		C->RegisterComponent();
		// Ilk ikisi yetiskin (HeightCm), digerleri cocuk (%70).
		const float H = I < 2 ? HeightCm : HeightCm * 0.7f;
		C->SetRelativeScale3D(FVector(W / 100.f, W / 100.f, H / 100.f));
		C->SetRelativeLocation(FVector(0.f, (I - 0.5f * (Members - 1)) * 45.f, 0.5f * H));
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, C))
		{
			MID->SetVectorParameterValue(TEXT("Tint"), Tint * (0.8f + 0.2f * I));
			C->SetMaterial(0, MID);
		}
		Figures.Add(C);
	}
}

FVector ASLFamily::GetSpeechAnchor() const
{
	return GetActorLocation() + GetActorUpVector() * (HeightCm + 40.f);
}

void ASLFamily::SetWaitingVisible(bool bVisible)
{
	bWaitingVisible = bVisible;
	SetActorHiddenInGame(!bVisible); // gizle, sifira olcekleme
}
