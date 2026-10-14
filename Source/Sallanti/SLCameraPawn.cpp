#include "SLCameraPawn.h"

#include "Camera/CameraComponent.h"

ASLCameraPawn::ASLCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;
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
	ApplyCamera();
}

void ASLCameraPawn::ApplyCamera()
{
	const FRotator ViewRot(Pitch, Yaw, 0.f);
	Camera->SetFieldOfView(FieldOfView);
	Camera->SetRelativeLocation(LookAtOffset - ViewRot.Vector() * Distance);
	Camera->SetRelativeRotation(ViewRot);
}
