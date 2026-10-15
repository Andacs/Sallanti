#include "SLFamily.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASLFamily::ASLFamily()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CardboardMat(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GhostMat(TEXT("/Game/Sallanti/Materials/M_Ghost.M_Ghost"));
	Cube = CubeMesh.Object;
	Sphere = SphereMesh.Object;
	Cylinder = CylMesh.Object;
	BaseMat = CardboardMat.Object;
	UnlitMat = GhostMat.Object;
	SetActorEnableCollision(false); // imlec isinini engellemesin; tiklama HUD'da ekran mesafesiyle
}

UStaticMeshComponent* ASLFamily::Part(USceneComponent* Parent, UStaticMesh* M, const FVector& Loc, const FVector& Size, const FLinearColor& C)
{
	UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
	P->SetStaticMesh(M);
	P->SetMobility(EComponentMobility::Movable);
	P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	P->SetupAttachment(Parent);
	P->RegisterComponent();
	P->SetRelativeScale3D(Size / 100.f);
	P->SetRelativeLocation(Loc);
	// Her parcaya ayri MID: ayni MID paylasilinca bir parcayi boyamak digerini boyuyordu (CLAUDE.md).
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, P))
	{
		MID->SetVectorParameterValue(TEXT("Tint"), C);
		P->SetMaterial(0, MID);
	}
	return P;
}

void ASLFamily::BuildFigures()
{
	if (!Cylinder || Pivots.Num() > 0)
	{
		return;
	}
	// Kume: yetiskinler arkada yan yana, cocuk(lar) onde ortada. Duz sira kapinin onundeki yolu kapatiyordu.
	const float W = FMath::Max(6.f, HeightCm * 0.24f);
	const FLinearColor Hair[3] = { FLinearColor(0.30f, 0.20f, 0.15f), FLinearColor(0.55f, 0.35f, 0.20f), FLinearColor(0.20f, 0.15f, 0.12f) };
	for (int32 I = 0; I < Members; ++I)
	{
		const bool bAdult = I < 2;
		const float H = bAdult ? HeightCm : HeightCm * 0.68f;
		const FVector Off = bAdult ? FVector(-0.4f * W, (I == 0 ? -0.6f : 0.6f) * W, 0.f)
			: FVector(0.55f * W, (I - 2 - 0.5f * (Members - 3)) * 0.9f * W, 0.f);
		USceneComponent* Pv = NewObject<USceneComponent>(this);
		Pv->SetMobility(EComponentMobility::Movable);
		Pv->SetupAttachment(GetRootComponent());
		Pv->RegisterComponent();
		Pv->SetRelativeLocation(Off);
		const float BodyH = 0.62f * H, HeadD = 0.36f * H;
		const FLinearColor Body = Tint * (bAdult ? (I == 0 ? 1.f : 0.85f) : 1.15f);
		Part(Pv, Cylinder, FVector(0, 0, 0.5f * BodyH), FVector(0.55f * W, 0.55f * W, BodyH), Body);
		USceneComponent* Head = NewObject<USceneComponent>(this);
		Head->SetMobility(EComponentMobility::Movable);
		Head->SetupAttachment(Pv);
		Head->RegisterComponent();
		Head->SetRelativeLocation(FVector(0, 0, BodyH + 0.45f * HeadD));
		Part(Head, Sphere, FVector::ZeroVector, FVector(HeadD, HeadD, HeadD), SkinColor);
		Part(Head, Sphere, FVector(-0.08f * HeadD, 0, 0.22f * HeadD), FVector(1.05f * HeadD, 1.05f * HeadD, 0.6f * HeadD), Hair[I % 3]);
		Pivots.Add(Pv);
		Heads.Add(Head);
		Offsets.Add(Off);
		Phase.Add(I * 1.7f);
	}
	// Secim halkasi: ayaklarin altinda isiksiz turkuaz disk.
	Ring = NewObject<UStaticMeshComponent>(this);
	Ring->SetStaticMesh(Cylinder);
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ring->SetCastShadow(false);
	Ring->SetupAttachment(GetRootComponent());
	Ring->RegisterComponent();
	Ring->SetRelativeScale3D(FVector(2.6f * W, 2.6f * W, 0.6f) / 100.f);
	Ring->SetRelativeLocation(FVector(0, 0, 1.2f));
	RingMID = UnlitMat ? UMaterialInstanceDynamic::Create(UnlitMat, Ring) : nullptr;
	if (RingMID)
	{
		RingMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.85f, 0.80f) * 2.f);
		RingMID->SetScalarParameterValue(TEXT("Opacity"), 0.55f);
		Ring->SetMaterial(0, RingMID);
	}
	Ring->SetVisibility(false);
}

FVector ASLFamily::GetSpeechAnchor() const
{
	return GetActorLocation() + GetActorUpVector() * (HeightCm + 40.f);
}

FVector ASLFamily::GetClickAnchor() const
{
	return GetActorLocation() + GetActorUpVector() * (0.55f * HeightCm);
}

void ASLFamily::SetWaitingVisible(bool bVisible)
{
	bWaitingVisible = bVisible;
	SetActorHiddenInGame(!bVisible); // gizle, sifira olcekleme
}

void ASLFamily::SetMood(float Happy01, bool bInSelected, bool bInHovered)
{
	if (Happy01 >= 0.999f && Happy < 0.999f)
	{
		Hop(); // mutlu oldugu an sevinc ziplamasi
	}
	Happy = Happy01;
	if (bInSelected != bSelected && Ring)
	{
		Ring->SetVisibility(bInSelected);
	}
	bSelected = bInSelected;
	bHovered = bInHovered;
}

void ASLFamily::WalkPath(const TArray<FVector>& Points, float InFinalYaw)
{
	Path = Points;
	PathIndex = 0;
	FinalYaw = InFinalYaw;
	bArrivedFlag = false;
}

void ASLFamily::Hop()
{
	HopTime = Time;
}

void ASLFamily::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	// Yuruyus: platform yerelinde sabit hizla, yuzu gidis yonune.
	const bool bWalking = IsWalking();
	if (bWalking)
	{
		const FVector Here = GetRootComponent()->GetRelativeLocation();
		const FVector Target = Path[PathIndex];
		const FVector Next = FMath::VInterpConstantTo(Here, Target, DeltaSeconds, WalkSpeed);
		const FVector D = Target - Here;
		if (D.SizeSquared2D() > 1.f)
		{
			const float Want = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
			const float Cur = GetRootComponent()->GetRelativeRotation().Yaw;
			GetRootComponent()->SetRelativeRotation(FRotator(0, FMath::FixedTurn(Cur, Want, 540.f * DeltaSeconds), 0));
		}
		GetRootComponent()->SetRelativeLocation(Next);
		if (FVector::DistSquared(Next, Target) < 1.f && ++PathIndex >= Path.Num())
		{
			GetRootComponent()->SetRelativeRotation(FRotator(0, FinalYaw, 0));
			bArrivedFlag = true;
		}
	}

	// Kendiliginden ziplama: mutluyken 2-4 sn'de bir, secilince surekli.
	if (!bWalking && (bSelected || Happy >= 0.999f) && Time > NextIdleHop)
	{
		Hop();
		NextIdleHop = Time + (bSelected ? 0.7f : FMath::FRandRange(2.f, 4.f));
	}
	const float HopT = (Time - HopTime) / 0.45f;
	const bool bSad = Happy < 0.5f;
	for (int32 I = 0; I < Pivots.Num(); ++I)
	{
		USceneComponent* Pv = Pivots[I];
		if (!Pv)
		{
			continue;
		}
		const float Ph = Phase[I];
		float Z = 0.f;
		float Roll = 0.f;
		if (bWalking)
		{
			Z = 2.5f * FMath::Abs(FMath::Sin(Time * 11.f + Ph)); // adim sekmesi
			Roll = 6.f * FMath::Sin(Time * 11.f + Ph);
		}
		else
		{
			const float Stagger = 0.06f * I; // ziplama figurler arasinda kucuk gecikmeyle: "dalga"
			const float T = HopT - Stagger / 0.45f;
			if (T >= 0.f && T < 1.f)
			{
				Z = HopHeightCm * 4.f * T * (1.f - T);
			}
			if (bHovered && I == 0)
			{
				Z = FMath::Max(Z, 3.f * FMath::Abs(FMath::Sin(Time * 9.f))); // el sallar gibi
			}
			Roll = bSad ? 4.f * FMath::Sin(Time * 1.2f + Ph) : 2.f * FMath::Sin(Time * 1.8f + Ph);
		}
		// Nefes: govde hafif uzayip kisalir (sifira yaklasmaz).
		const float Breath = 1.f + 0.03f * FMath::Sin(Time * 2.2f + Ph);
		Pv->SetRelativeLocation(Offsets[I] + FVector(0, 0, Z));
		Pv->SetRelativeRotation(FRotator(0, 0, Roll));
		Pv->SetRelativeScale3D(FVector(1.f, 1.f, Breath));
		if (USceneComponent* H = Heads[I])
		{
			H->SetRelativeRotation(FRotator(bSad ? -18.f : 0.f, 0, 0)); // mutsuz: bas one egik
		}
	}
	if (Ring && bSelected && RingMID)
	{
		RingMID->SetScalarParameterValue(TEXT("Opacity"), 0.4f + 0.2f * FMath::Sin(Time * 4.f));
	}
}
