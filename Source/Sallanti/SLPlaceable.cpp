#include "SLPlaceable.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogSLPlaceable, Log, All);

ASLPlaceable::ASLPlaceable()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // yalnizca aile yururken

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
	const float HeightCm = bFlat ? FlatHeightCm : SizeCells.Z * GridSize;
	const FVector SizeCm(SizeCells.X * GridSize, SizeCells.Y * GridSize, HeightCm);
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
	BuildEntrance();
}

UStaticMeshComponent* ASLPlaceable::MakePart(const FLinearColor& Color, bool bForGhost)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(DefaultCube);
	C->SetMobility(EComponentMobility::Movable);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetupAttachment(GetRootComponent());
	C->RegisterComponent();
	// Temel materyalden ayri ornek: evin MID'ini vermek ayni MID'i paylastirip evi boyuyordu.
	UMaterialInterface* BaseMat = Mesh->GetMaterial(0);
	if (const UMaterialInstanceDynamic* Own = Cast<UMaterialInstanceDynamic>(BaseMat))
	{
		BaseMat = Own->Parent;
	}
	if (UMaterialInstanceDynamic* PartMID = UMaterialInstanceDynamic::Create(BaseMat, C))
	{
		PartMID->SetVectorParameterValue(bForGhost ? TEXT("Color") : TEXT("Tint"), Color);
		C->SetMaterial(0, PartMID);
	}
	return C;
}

void ASLPlaceable::BuildEntrance()
{
	if (!bHasEntrance || !DefaultCube || !GetWorld())
	{
		return;
	}
	if (DoorMesh)
	{
		DoorMesh->DestroyComponent();
		DoorMesh = nullptr;
	}
	for (UStaticMeshComponent* M : EntranceMarks)
	{
		if (M)
		{
			M->DestroyComponent();
		}
	}
	EntranceMarks.Reset();

	// Kapi yerel +X yuzunde, ortada. Boyut yuze gore: evde 30x55, AVM'de 45x80 civari.
	const float HalfX = 0.5f * SizeCells.X * GridSize;
	const float FaceW = SizeCells.Y * GridSize;
	const float BodyH = SizeCells.Z * GridSize;
	const float DoorW = FMath::Clamp(FaceW * 0.3f, 25.f, 60.f);
	const float DoorH = FMath::Min(BodyH * 0.55f, 90.f);
	DoorMesh = MakePart(bGhost ? FLinearColor(0.f, 0.f, 0.f) : DoorColor, bGhost);
	DoorMesh->SetRelativeScale3D(FVector(4.f / 100.f, DoorW / 100.f, DoorH / 100.f));
	DoorMesh->SetRelativeLocation(FVector(HalfX + 1.f, 0.f, 0.5f * DoorH));

	// Kapi onu hucreleri: yuzun ortasi; cift genislikte ortadaki iki hucre.
	const int32 W = SizeCells.Y;
	TArray<float> Ys;
	if (W % 2) { Ys.Add(0.f); } else { Ys.Add(-0.5f * GridSize); Ys.Add(0.5f * GridSize); }
	for (float Y : Ys)
	{
		UStaticMeshComponent* M = MakePart(EntranceMarkColor, false);
		M->SetRelativeScale3D(FVector(0.7f * GridSize / 100.f, 0.7f * GridSize / 100.f, 0.6f / 100.f));
		M->SetRelativeLocation(FVector(HalfX + 0.5f * GridSize, Y, 0.4f)); // yuzeyin hemen ustu; yol (4 cm) ortuyor
		M->SetVisibility(false);
		EntranceMarks.Add(M);
	}
}

void ASLPlaceable::SetEntranceMarkersVisible(bool bVisible)
{
	for (UStaticMeshComponent* M : EntranceMarks)
	{
		if (M && M->IsVisible() != bVisible)
		{
			M->SetVisibility(bVisible);
		}
	}
}

FIntPoint ASLPlaceable::GetFacingDir() const
{
	switch (Facing & 3)
	{
	case 0: return FIntPoint(1, 0);
	case 1: return FIntPoint(0, 1);
	case 2: return FIntPoint(-1, 0);
	default: return FIntPoint(0, -1);
	}
}

TArray<FIntPoint> ASLPlaceable::GetEntranceCells() const
{
	TArray<FIntPoint> Out;
	if (!bHasEntrance)
	{
		return Out;
	}
	const FIntPoint F = GetFootprint();
	const FIntPoint D = GetFacingDir();
	// Kapinin yuzu boyunca genislik ve o yuzun ortasindaki hucre(ler).
	const int32 W = D.X != 0 ? F.Y : F.X;
	TArray<int32> Mid;
	if (W % 2) { Mid.Add(W / 2); } else { Mid.Add(W / 2 - 1); Mid.Add(W / 2); }
	for (int32 M : Mid)
	{
		if (D.X > 0)      Out.Add(FIntPoint(CellMin.X + F.X, CellMin.Y + M));
		else if (D.X < 0) Out.Add(FIntPoint(CellMin.X - 1, CellMin.Y + M));
		else if (D.Y > 0) Out.Add(FIntPoint(CellMin.X + M, CellMin.Y + F.Y));
		else              Out.Add(FIntPoint(CellMin.X + M, CellMin.Y - 1));
	}
	return Out;
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

void ASLPlaceable::SetHeld(bool bInHeld)
{
	bHeld = bInHeld;
	// Gizle, sifira olcekleme (04_UNREAL_TUZAKLARI: sifir olcek -> NaN -> donma).
	SetActorHiddenInGame(bInHeld);
	SetActorEnableCollision(!bInHeld); // elde tutulan, imlec isinini engellemesin
}

void ASLPlaceable::StartResidentsArrival(const FVector& StartPlatformLocal)
{
	if (bResidentsStarted || Residents <= 0 || !DefaultCube)
	{
		return;
	}
	bResidentsStarted = true;

	// Baslangic noktasi bina yereline: bina tasinsa da aile onunla gider.
	const FTransform Rel = GetRootComponent()->GetRelativeTransform();
	const FVector Start = Rel.InverseTransformPosition(StartPlatformLocal);
	// Hedef: kapinin (yerel +X yuzu) 30 cm onu. Kapisiz binada aileye en yakin yuz.
	const FVector2D Dir2 = FVector2D(Start.X, Start.Y).GetSafeNormal();
	const float HalfX = 0.5f * SizeCells.X * GridSize, HalfY = 0.5f * SizeCells.Y * GridSize;
	const FVector Door = bHasEntrance ? FVector(HalfX + 30.f, 0.f, 0.f)
		: FMath::Abs(Dir2.X) >= FMath::Abs(Dir2.Y)
			? FVector(FMath::Sign(Dir2.X) * (HalfX + 30.f), 0.f, 0.f)
			: FVector(0.f, FMath::Sign(Dir2.Y) * (HalfY + 30.f), 0.f);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Door.GetSafeNormal());

	// Yol: kapi aileye donukse dosdogru; degilse once yan sokaga (bina yan yuzunden 60 cm), sonra kapinin
	// onune, sonra kapiya. Boylece aile binanin icinden yurumez. (Kapisiz binada hep dosdogru.)
	ResidentPath.Reset();
	if (bHasEntrance && Start.X < HalfX + 30.f)
	{
		const float S = Start.Y >= 0.f ? 1.f : -1.f;
		const float Lane = S * FMath::Max(FMath::Abs(Start.Y), HalfY + 60.f);
		ResidentPath.Add(FVector(Start.X, Lane, 0.f));
		ResidentPath.Add(FVector(HalfX + 60.f, Lane, 0.f));
	}
	ResidentPath.Add(Door);

	// Figurlere evin MID'i verilirse motor yeni ornek acmiyor, ayni MID'i paylastiriyor: figuru boyamak
	// evi de boyuyordu (ekranda goruldu, ev mavi). Temel materyalden ayri ornek ac.
	UMaterialInterface* BaseMat = Mesh->GetMaterial(0);
	if (const UMaterialInstanceDynamic* HouseMID = Cast<UMaterialInstanceDynamic>(BaseMat))
	{
		BaseMat = HouseMID->Parent;
	}

	const float W = 30.f, H = ResidentHeightCm;
	for (int32 I = 0; I < Residents; ++I)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(DefaultCube);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(GetRootComponent());
		C->RegisterComponent();
		C->SetRelativeScale3D(FVector(W / 100.f, W / 100.f, H / 100.f));
		const float Spread = (I - 0.5f * (Residents - 1)) * 45.f;
		C->SetRelativeLocation(Start + Side * Spread + FVector(0.f, 0.f, 0.5f * H));
		if (UMaterialInstanceDynamic* RMID = UMaterialInstanceDynamic::Create(BaseMat, C))
		{
			RMID->SetVectorParameterValue(TEXT("Tint"), ResidentTint * (0.8f + 0.2f * I));
			C->SetMaterial(0, RMID);
		}
		ResidentMeshes.Add(C);
		ResidentOffsets.Add(Side * Spread + FVector(0.f, 0.f, 0.5f * H));
		ResidentStep.Add(0);
	}
	SetActorTickEnabled(true);
	UE_LOG(LogSLPlaceable, Log, TEXT("SL_FAMILY %s: %d kisi yola cikti, kapi %s, %d durak"), *TypeId.ToString(), Residents, *Door.ToString(), ResidentPath.Num());
}

void ASLPlaceable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bResidentsStarted || bResidentsArrived)
	{
		SetActorTickEnabled(false);
		return;
	}
	bool bAll = true;
	for (int32 I = 0; I < ResidentMeshes.Num(); ++I)
	{
		UStaticMeshComponent* C = ResidentMeshes[I];
		if (!C || !C->IsVisible())
		{
			continue;
		}
		const FVector Target = ResidentPath[ResidentStep[I]] + ResidentOffsets[I];
		const FVector Next = FMath::VInterpConstantTo(C->GetRelativeLocation(), Target, DeltaSeconds, ResidentWalkSpeed);
		C->SetRelativeLocation(Next);
		bAll = false;
		if (FVector::DistSquared(Next, Target) < 1.f)
		{
			if (ResidentStep[I] + 1 < ResidentPath.Num())
			{
				++ResidentStep[I]; // sonraki durak
			}
			else
			{
				C->SetVisibility(false); // kapiya vardi: eve girdi (gizle, sifira olcekleme)
			}
		}
	}
	if (bAll)
	{
		bResidentsArrived = true;
		UE_LOG(LogSLPlaceable, Log, TEXT("SL_FAMILY %s: aile eve yerlesti"), *TypeId.ToString());
	}
}
