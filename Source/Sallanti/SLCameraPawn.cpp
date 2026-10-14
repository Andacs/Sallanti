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

void ASLCameraPawn::PanByScreen(float DX, float DY, float ViewportW)
{
	// Bakis noktasindaki 1 piksel = 2 * D * tan(FOV/2) / ekran genisligi (cm). Dikeyde zemin egik
	// goruldugu icin 1/sin(pitch) kadar uzar. "Tutup cekme" hissi: harita imlecle ayni yone gider.
	const float CmPerPx = 2.f * Distance * FMath::Tan(FMath::DegreesToRadians(0.5f * FieldOfView)) / FMath::Max(ViewportW, 1.f);
	const float YawRad = FMath::DegreesToRadians(Yaw);
	const FVector2D Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad));
	const FVector2D Right(-FMath::Sin(YawRad), FMath::Cos(YawRad));
	const float VScale = 1.f / FMath::Max(0.2f, FMath::Abs(FMath::Sin(FMath::DegreesToRadians(Pitch))));
	PanOffset += (-DX * Right + DY * VScale * Fwd) * CmPerPx * PanSpeed;
	PanOffset.X = FMath::Clamp(PanOffset.X, -PanLimit, PanLimit);
	PanOffset.Y = FMath::Clamp(PanOffset.Y, -PanLimit, PanLimit);
	ApplyCamera();
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
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = ExposureBias;
	// Hareket bulanikligi kapali: sag tikla kaydirmada ve zoom'da tahta lekeleniyordu (ekran goruntusu).
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;
	Camera->PostProcessBlendWeight = 1.f;
	Camera->SetRelativeLocation(LookAtOffset + FVector(PanOffset, 0.f) - ViewRot.Vector() * Distance);
	Camera->SetRelativeRotation(ViewRot);
}
