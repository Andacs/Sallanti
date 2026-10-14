#include "SLCameraPawn.h"

#include "Camera/CameraComponent.h"

ASLCameraPawn::ASLCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->ProjectionMode = ECameraProjectionMode::Perspective; // ortho'nun golge tuhafliklari var
	Camera->bUsePawnControlRotation = false;
	ApplyCamera();
}

void ASLCameraPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCamera();
}

void ASLCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	TargetDistance = FMath::Clamp(Distance, ZoomMin, ZoomMax);
	ApplyCamera();
}

void ASLCameraPawn::Zoom(float Ticks)
{
	// Carpimsal adim: her tik uzakligin sabit bir orani, yakin ve uzakta ayni "his".
	TargetDistance = FMath::Clamp(TargetDistance * FMath::Pow(1.f - ZoomStepFraction, Ticks), ZoomMin, ZoomMax);
}

void ASLCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!FMath::IsNearlyEqual(Distance, TargetDistance, 0.5f))
	{
		Distance = FMath::FInterpTo(Distance, TargetDistance, DeltaSeconds, ZoomInterpSpeed);
		ApplyCamera();
	}
}

void ASLCameraPawn::ApplyCamera()
{
	const FRotator ViewRot(Pitch, Yaw, 0.f);
	Camera->SetFieldOfView(FieldOfView);
	Camera->SetRelativeLocation(LookAtOffset - ViewRot.Vector() * Distance);
	Camera->SetRelativeRotation(ViewRot);
}
