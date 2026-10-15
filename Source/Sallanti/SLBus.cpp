#include "SLBus.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASLBus::ASLBus()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Cardboard(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	Cube = CubeMesh.Object;
	Cylinder = CylMesh.Object;
	Mat = Cardboard.Object;
	SetActorEnableCollision(false);
}

UStaticMeshComponent* ASLBus::Part(UStaticMesh* M, const FVector& Loc, const FVector& Size, const FLinearColor& C, const FRotator& R)
{
	UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
	P->SetStaticMesh(M);
	P->SetMobility(EComponentMobility::Movable);
	P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	P->SetupAttachment(Body);
	P->RegisterComponent();
	P->SetRelativeRotation(R);
	P->SetRelativeScale3D(Size / 100.f);
	P->SetRelativeLocation(Loc);
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, P))
	{
		MID->SetVectorParameterValue(TEXT("Tint"), C);
		P->SetMaterial(0, MID);
	}
	return P;
}

void ASLBus::BuildBody()
{
	bBuilt = true;
	Body = NewObject<USceneComponent>(this);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetupAttachment(GetRootComponent());
	Body->RegisterComponent();
	if (BodyMesh)
	{
		UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
		M->SetStaticMesh(BodyMesh);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetupAttachment(Body);
		M->RegisterComponent();
		return;
	}
	// Yol karosu 42 cm: otobus 36 x 18 x 17, taban yolun ustunde (yol 2.5 cm).
	const float L = 36.f, W = 18.f, H = 14.f, Wheel = 6.f;
	const float Z0 = 2.5f + 0.5f * Wheel;
	Part(Cube, FVector(0, 0, Z0 + 0.5f * H), FVector(L, W, H), BodyColor);
	Part(Cube, FVector(0, 0, Z0 + H + 1.f), FVector(L - 4.f, W - 2.f, 2.f), FLinearColor(0.99f, 0.96f, 0.90f)); // tavan
	Part(Cube, FVector(0, 0, Z0 + 0.62f * H), FVector(L - 6.f, W + 0.6f, 0.3f * H), FLinearColor(0.42f, 0.60f, 0.86f)); // yan camlar
	Part(Cube, FVector(0.5f * L + 0.3f, 0, Z0 + 0.62f * H), FVector(0.6f, W - 4.f, 0.32f * H), FLinearColor(0.42f, 0.60f, 0.86f)); // on cam
	Part(Cube, FVector(0, 0, Z0 + 0.22f * H), FVector(L + 0.4f, W + 0.4f, 0.12f * H), StripeColor); // serit
	for (int32 I = 0; I < 4; ++I)
	{
		const float X = (I < 2 ? 0.3f : -0.3f) * L;
		const float Y = (I % 2 ? 0.5f : -0.5f) * W;
		Part(Cylinder, FVector(X, Y, 2.5f + 0.5f * Wheel), FVector(Wheel, Wheel, 3.f), FLinearColor(0.25f, 0.22f, 0.25f), FRotator(0, 0, 90.f));
	}
}

void ASLBus::SetRoute(const TArray<FVector>& Points, const TArray<int32>& Stops)
{
	if (!bBuilt)
	{
		BuildBody();
	}
	if (Points == Route && StopSet.Num() == Stops.Num())
	{
		return;
	}
	const FVector Here = GetRootComponent()->GetRelativeLocation();
	const bool bFirst = Route.Num() == 0;
	Route = Points;
	StopSet = TSet<int32>(Stops);
	if (Route.Num() == 0)
	{
		SetActorHiddenInGame(true);
		return;
	}
	SetActorHiddenInGame(false);
	// Yeni rotada en yakin noktadan devam: yol eklenince otobus zipla(ya)masin.
	int32 Best = 0;
	if (!bFirst)
	{
		float BestD = TNumericLimits<float>::Max();
		for (int32 I = 0; I < Route.Num(); ++I)
		{
			const float D = FVector::DistSquared2D(Route[I], Here);
			if (D < BestD)
			{
				BestD = D;
				Best = I;
			}
		}
	}
	else if (Stops.Num() > 0)
	{
		Best = Stops[0]; // ilk kez: duraktan baslar
	}
	Index = Best;
	Alpha = 0.f;
	Dir = Index >= Route.Num() - 1 ? -1 : 1;
	WaitLeft = StopSet.Contains(Index) ? StopSeconds : 0.f;
	bWaitIsStop = WaitLeft > 0.f;
	if (Route.Num() > 1)
	{
		const FVector D = Route[Index + Dir] - Route[Index];
		Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
	}
	SetActorRelativeLocation(Route[Index]);
	SetActorRelativeRotation(FRotator(0, Yaw, 0));
}

void ASLBus::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Route.Num() < 2)
	{
		return;
	}
	if (WaitLeft > 0.f)
	{
		WaitLeft -= DeltaSeconds;
		return;
	}
	const int32 Next = Index + Dir;
	const FVector A = Route[Index], B = Route[Next];
	const FVector Seg = B - A;
	const float Len = FMath::Max(1.f, Seg.Size2D());

	// Once burun gidecegi yone donsun (viraj ve uc donusu), sonra ilerlesin.
	const float WantYaw = FMath::RadiansToDegrees(FMath::Atan2(Seg.Y, Seg.X));
	const float Delta = FMath::FindDeltaAngleDegrees(Yaw, WantYaw);
	if (FMath::Abs(Delta) > 1.f)
	{
		Yaw += FMath::Clamp(Delta, -TurnRate * DeltaSeconds, TurnRate * DeltaSeconds);
		SetActorRelativeRotation(FRotator(0, Yaw, 0));
		if (FMath::Abs(Delta) > 30.f)
		{
			return; // keskin donuste yerinde don
		}
	}
	Alpha += Speed * DeltaSeconds / Len;
	if (Alpha >= 1.f)
	{
		Alpha = 0.f;
		Index = Next;
		if (Index == 0 || Index == Route.Num() - 1)
		{
			Dir = -Dir; // uca vardi: geri don
			WaitLeft = TurnSeconds;
			bWaitIsStop = false;
		}
		if (StopSet.Contains(Index))
		{
			WaitLeft = FMath::Max(WaitLeft, StopSeconds);
			bWaitIsStop = true;
		}
		SetActorRelativeLocation(Route[Index]);
		return;
	}
	SetActorRelativeLocation(FMath::Lerp(A, B, Alpha));
}
