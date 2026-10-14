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
	PrimaryActorTick.bStartWithTickEnabled = false; // yalnizca aile yururken ya da efekt oynarken

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetSimulatePhysics(false);

	// Plaka da isini durdurur: binanin plakasina tiklamak da binayi kaldirir.
	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(Root);
	Plate->SetMobility(EComponentMobility::Movable);
	Plate->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CardboardMat(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GhostMat(TEXT("/Game/Sallanti/Materials/M_Ghost.M_Ghost"));
	DefaultCube = CubeMesh.Object;
	DiscMesh = CylMesh.Object;
	FxMaterial = GhostMat.Object; // yari saydam, Color + Opacity: halka icin yeterli, yeni asset gerekmiyor
	if (CubeMesh.Succeeded())
	{
		Plate->SetStaticMesh(CubeMesh.Object);
	}
	if (CardboardMat.Succeeded())
	{
		Mesh->SetMaterial(0, CardboardMat.Object);
		Plate->SetMaterial(0, CardboardMat.Object);
	}
}

void ASLPlaceable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisuals();
}

void ASLPlaceable::ApplyVisuals()
{
	const float PX = PlateSizeX(), PY = PlateSizeY();
	if (bFlat)
	{
		// Yol: plakasiz tek karo, karo boyunda; uzantilar derzi kapatir.
		Plate->SetVisibility(false);
		Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetStaticMesh(DefaultCube);
		Mesh->SetRelativeScale3D(FVector(PX, PY, FlatHeightCm) / 100.f);
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, 0.5f * FlatHeightCm));
	}
	else
	{
		Plate->SetVisibility(true);
		Plate->SetRelativeScale3D(FVector(PX, PY, PlateHeightCm) / 100.f);
		Plate->SetRelativeLocation(FVector(0.f, 0.f, 0.5f * PlateHeightCm));
		const float H = SizeCells.Z * GridSize;
		if (DisplayMesh)
		{
			// Modelcinin mesh'i: olceklenmez, plakanin ustune oturur (pivot taban ortasi).
			Mesh->SetStaticMesh(DisplayMesh);
			Mesh->SetRelativeScale3D(FVector::OneVector);
			Mesh->SetRelativeLocation(FVector(0.f, 0.f, PlateHeightCm));
		}
		else
		{
			// Motor kupu 100 cm, merkez pivotlu: govde boyutuna olcekle, plakanin ustune kaldir.
			Mesh->SetStaticMesh(DefaultCube);
			Mesh->SetRelativeScale3D(FVector(2.f * BodyHalfX(), 2.f * BodyHalfY(), H) / 100.f);
			Mesh->SetRelativeLocation(FVector(0.f, 0.f, PlateHeightCm + 0.5f * H));
		}
	}
	BodyBaseScale = Mesh->GetRelativeScale3D();

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
		if (!bFlat)
		{
			if (!PlateMID)
			{
				PlateMID = Plate->CreateDynamicMaterialInstance(0);
			}
			if (PlateMID)
			{
				PlateMID->SetVectorParameterValue(TEXT("Tint"), PlateColor);
			}
		}
	}
	BuildEntrance();
	BuildRoadLinks();
}

UStaticMeshComponent* ASLPlaceable::MakePart(const FLinearColor& Color, bool bForGhost, UStaticMesh* InMesh)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(InMesh ? InMesh : DefaultCube.Get());
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

	if (!DisplayMesh)
	{
		// Kapi govdenin yerel +X yuzunde, ortada; boyu ve eni govdeye gore.
		const float FaceW = 2.f * BodyHalfY();
		const float BodyH = SizeCells.Z * GridSize;
		const float DoorW = FMath::Clamp(FaceW * 0.35f, 8.f, 60.f);
		const float DoorH = FMath::Min(BodyH * 0.55f, 90.f);
		DoorMesh = MakePart(bGhost ? FLinearColor(0.f, 0.f, 0.f) : DoorColor, bGhost);
		DoorMesh->SetRelativeScale3D(FVector(2.f / 100.f, DoorW / 100.f, DoorH / 100.f));
		DoorMesh->SetRelativeLocation(FVector(BodyHalfX() + 1.f, 0.f, PlateHeightCm + 0.5f * DoorH));
	}

	// Giris yolu (kaldirim): kapidan plaka kenarini asip derzin ortasina kadar; orada yolun uzantisiyla
	// bulusur. Tek genislikte kapi eninin ~2 kati; cift genislikte iki giris hucresini de tutacak kadar.
	for (UStaticMeshComponent* P : EntrancePaths)
	{
		if (P)
		{
			P->DestroyComponent();
		}
	}
	EntrancePaths.Reset();
	if (!bGhost)
	{
		const float From = BodyHalfX();
		const float To = 0.5f * PlateSizeX() + 0.5f * TileGap + 1.f;
		const float PathW = (SizeCells.Y % 2) ? FMath::Clamp(2.f * BodyHalfY() * 0.6f, 14.f, 40.f) : GridSize;
		const float PathH = PlateHeightCm + 0.8f;
		UStaticMeshComponent* Path = MakePart(EntrancePathColor, false);
		Path->SetRelativeScale3D(FVector(To - From, PathW, PathH) / 100.f);
		Path->SetRelativeLocation(FVector(0.5f * (From + To), 0.f, 0.5f * PathH));
		Path->SetVisibility(bEntranceConnected);
		EntrancePaths.Add(Path);
	}

	// Kapi onu hucreleri: yuzun ortasi; cift genislikte ortadaki iki hucre. Karonun ustunde, yolun altinda.
	const float HalfX = 0.5f * SizeCells.X * GridSize;
	const int32 W = SizeCells.Y;
	TArray<float> Ys;
	if (W % 2) { Ys.Add(0.f); } else { Ys.Add(-0.5f * GridSize); Ys.Add(0.5f * GridSize); }
	for (float Y : Ys)
	{
		UStaticMeshComponent* M = MakePart(EntranceMarkColor, false);
		M->SetRelativeScale3D(FVector(0.7f * GridSize / 100.f, 0.7f * GridSize / 100.f, 0.6f / 100.f));
		M->SetRelativeLocation(FVector(HalfX + 0.5f * GridSize, Y, 1.0f)); // karo 0.6, yol 2.5: arada
		M->SetVisibility(false);
		EntranceMarks.Add(M);
	}
}

void ASLPlaceable::BuildRoadLinks()
{
	for (UStaticMeshComponent* L : RoadLinks)
	{
		if (L)
		{
			L->DestroyComponent();
		}
	}
	RoadLinks.Reset();
	RoadLinkMIDs.Reset();
	if (!bFlat || bGhost || !GetWorld())
	{
		return;
	}
	// Derzin yarisi + 1 cm bindirme: iki komsunun uzantisi derzin ortasinda bulusur, bosluk kalmaz.
	const float Core = PlateSizeX();
	const float Len = 0.5f * TileGap + 1.f;
	const FVector2D Dirs[4] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
	for (int32 I = 0; I < 4; ++I)
	{
		UStaticMeshComponent* L = MakePart(Tint, false);
		const bool bAlongX = Dirs[I].X != 0;
		L->SetRelativeScale3D(FVector(bAlongX ? Len : Core, bAlongX ? Core : Len, FlatHeightCm) / 100.f);
		const float Off = 0.5f * Core + 0.5f * Len - 0.5f;
		L->SetRelativeLocation(FVector(Dirs[I].X * Off, Dirs[I].Y * Off, 0.5f * FlatHeightCm));
		L->SetVisibility((RoadLinkMask >> I) & 1);
		RoadLinks.Add(L);
		RoadLinkMIDs.Add(Cast<UMaterialInstanceDynamic>(L->GetMaterial(0)));
	}
}

void ASLPlaceable::SetEntranceConnected(bool bConnected)
{
	bEntranceConnected = bConnected;
	for (UStaticMeshComponent* P : EntrancePaths)
	{
		if (P)
		{
			P->SetVisibility(bConnected);
		}
	}
}

void ASLPlaceable::BuildRoadShape()
{
	for (UStaticMeshComponent* S : RoadShape)
	{
		if (S)
		{
			S->DestroyComponent();
		}
	}
	RoadShape.Reset();
	RoadShapeMIDs.Reset();
	RoadShapeMask = RoadLinkMask;
	if (!bFlat || bGhost || !GetWorld())
	{
		return; // bina/hayalet: govdeye dokunma (olculdu: tasinan binanin govdesi gizli kaliyordu)
	}
	const float Core = PlateSizeX();        // yol eni = karo eni
	const float Half = 0.5f * GridSize;      // hucre yarisi
	const float Z = 0.5f * FlatHeightCm;
	auto AddPiece = [this](UStaticMesh* InMesh) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* P = MakePart(Tint, false, InMesh);
		RoadShape.Add(P);
		RoadShapeMIDs.Add(Cast<UMaterialInstanceDynamic>(P->GetMaterial(0)));
		return P;
	};
	const FVector2D Dirs[4] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
	TArray<int32> On;
	for (int32 I = 0; I < 4; ++I)
	{
		if ((RoadLinkMask >> I) & 1)
		{
			On.Add(I);
		}
	}

	const bool bCorner = On.Num() == 2 && (On[1] - On[0]) % 2 == 1; // komsu bitler = dik iki yon
	if (bCorner)
	{
		// Viraj: iki kenar ortasini birlestiren ceyrek daire. Merkez iki yonun kosesinde (a+b)*Half,
		// yaricap Half; yol eni Core. Kare karo gizlenir (collision kalir, imlec yine tutar).
		const FVector2D A = Dirs[On[0]], B = Dirs[On[1]];
		const FVector2D C = (A + B) * Half;
		const float Start = FMath::Atan2(-B.Y, -B.X); // C'den A kenar ortasina: -B yonu
		float End = FMath::Atan2(-A.Y, -A.X);
		float Sweep = FMath::FindDeltaAngleRadians(Start, End);
		const int32 N = FMath::Max(2, CurveSegments);
		const float Step = Sweep / N;
		// Parca boyu dis kenara gore: merkez cizgisine gore olunca dis kenarda dis dis bosluk kaliyordu.
		const float Chord = 2.f * (Half + 0.5f * Core) * FMath::Sin(FMath::Abs(Step) * 0.5f) + 1.f;
		for (int32 I = 0; I < N; ++I)
		{
			const float T = Start + Step * (I + 0.5f);
			const FVector2D P = C + Half * FVector2D(FMath::Cos(T), FMath::Sin(T));
			UStaticMeshComponent* Seg = AddPiece(nullptr);
			Seg->SetRelativeScale3D(FVector(Chord, Core, FlatHeightCm) / 100.f);
			Seg->SetRelativeLocation(FVector(P.X, P.Y, Z));
			Seg->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(T) + 90.f, 0.f));
		}
		Mesh->SetVisibility(false); // collision kalir: imlec yine tutar
	}
	else if (On.Num() == 1 && DiscMesh)
	{
		// Cikmaz: uzanti tarafina yarim karo + ortada disk = yuvarlak uc (Blender taslagindaki gibi).
		const FVector2D D = Dirs[On[0]];
		const float HalfCore = 0.5f * Core;
		UStaticMeshComponent* HalfBox = AddPiece(nullptr);
		const bool bAlongX = D.X != 0;
		HalfBox->SetRelativeScale3D(FVector(bAlongX ? HalfCore : Core, bAlongX ? Core : HalfCore, FlatHeightCm) / 100.f);
		HalfBox->SetRelativeLocation(FVector(D.X * 0.5f * HalfCore, D.Y * 0.5f * HalfCore, Z));
		UStaticMeshComponent* Disc = AddPiece(DiscMesh);
		Disc->SetRelativeScale3D(FVector(Core, Core, FlatHeightCm) / 100.f);
		Disc->SetRelativeLocation(FVector(0.f, 0.f, Z));
		Mesh->SetVisibility(false);
	}
	else
	{
		Mesh->SetVisibility(true); // duz, T, kavsak, tek basina: kare karo (eldeyken aktor zaten gizli)
	}
}

void ASLPlaceable::SetRoadLinks(uint8 Mask)
{
	RoadLinkMask = Mask;
	if (RoadShapeMask != Mask)
	{
		BuildRoadShape();
	}
	for (int32 I = 0; I < RoadLinks.Num(); ++I)
	{
		if (RoadLinks[I])
		{
			RoadLinks[I]->SetVisibility((Mask >> I) & 1);
		}
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
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Plate->SetCastShadow(false);
	BuildRoadLinks(); // hayalette uzanti yok
	if (GhostMaterial)
	{
		Mesh->SetMaterial(0, GhostMaterial);
		MID = Mesh->CreateDynamicMaterialInstance(0, GhostMaterial);
		Plate->SetMaterial(0, GhostMaterial);
		PlateMID = Plate->CreateDynamicMaterialInstance(0, GhostMaterial);
	}
}

void ASLPlaceable::SetGhostColor(const FLinearColor& Color, float Opacity)
{
	if (bGhost && MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
	if (bGhost && PlateMID)
	{
		PlateMID->SetVectorParameterValue(TEXT("Color"), Color);
		PlateMID->SetScalarParameterValue(TEXT("Opacity"), Opacity * 0.8f);
	}
}

void ASLPlaceable::SetHeld(bool bInHeld)
{
	bHeld = bInHeld;
	// Gizle, sifira olcekleme (04_UNREAL_TUZAKLARI: sifir olcek -> NaN -> donma).
	SetActorHiddenInGame(bInHeld);
	SetActorEnableCollision(!bInHeld); // elde tutulan, imlec isinini engellemesin
	if (bInHeld && bFlat)
	{
		SetRoadLinks(0);
	}
}

// --- Baglanti efekti -----------------------------------------------------------------------------

void ASLPlaceable::PlayConnectFx(float DelaySeconds, bool bRing)
{
	if (bGhost)
	{
		return;
	}
	FxTime = -FMath::Max(0.f, DelaySeconds);
	bFxRing = bRing;
	if (bRing && !FxRing && DiscMesh && FxMaterial)
	{
		FxRing = NewObject<UStaticMeshComponent>(this);
		FxRing->SetStaticMesh(DiscMesh);
		FxRing->SetMobility(EComponentMobility::Movable);
		FxRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FxRing->SetCastShadow(false);
		FxRing->SetupAttachment(GetRootComponent());
		FxRing->RegisterComponent();
		FxRingMID = UMaterialInstanceDynamic::Create(FxMaterial, FxRing);
		FxRing->SetMaterial(0, FxRingMID);
	}
	if (FxRing)
	{
		FxRing->SetVisibility(false);
	}
	SetActorTickEnabled(true);
}

void ASLPlaceable::SetSurfaceTint(const FLinearColor& C)
{
	if (bFlat)
	{
		if (MID) { MID->SetVectorParameterValue(TEXT("Tint"), C); }
		for (UMaterialInstanceDynamic* L : RoadLinkMIDs)
		{
			if (L) { L->SetVectorParameterValue(TEXT("Tint"), C); }
		}
		for (UMaterialInstanceDynamic* L : RoadShapeMIDs)
		{
			if (L) { L->SetVectorParameterValue(TEXT("Tint"), C); }
		}
	}
	else if (PlateMID)
	{
		PlateMID->SetVectorParameterValue(TEXT("Tint"), C);
	}
}

bool ASLPlaceable::TickFx(float DeltaSeconds)
{
	if (FxTime <= FxNone)
	{
		return false;
	}
	FxTime += DeltaSeconds;
	if (FxTime < 0.f)
	{
		return true; // gecikmede
	}
	const float T = FMath::Clamp(FxTime / FMath::Max(ConnectFxSeconds, 0.05f), 0.f, 1.f);
	// Parlama: hizli yukselir, yavas soner.
	const float Glow = T < 0.2f ? T / 0.2f : 1.f - (T - 0.2f) / 0.8f;
	const FLinearColor Base = bFlat ? Tint : PlateColor;
	SetSurfaceTint(FMath::Lerp(Base, ConnectFxColor, FMath::Clamp(Glow, 0.f, 1.f)));
	if (!bFlat)
	{
		// Govde kucuk bir "zipla": olcek 1 -> 1.08 -> 1 (sifira yaklasmaz).
		Mesh->SetRelativeScale3D(BodyBaseScale * (1.f + 0.08f * FMath::Sin(T * UE_PI)));
	}
	if (FxRing && bFxRing)
	{
		// Halka: plakadan disari yayilir, solar.
		const float D = FMath::Max(PlateSizeX(), PlateSizeY()) * FMath::Lerp(1.f, 2.4f, T);
		FxRing->SetVisibility(true);
		FxRing->SetRelativeScale3D(FVector(D / 100.f, D / 100.f, 0.02f));
		FxRing->SetRelativeLocation(FVector(0.f, 0.f, 3.f));
		if (FxRingMID)
		{
			FxRingMID->SetVectorParameterValue(TEXT("Color"), ConnectFxColor);
			FxRingMID->SetScalarParameterValue(TEXT("Opacity"), 0.6f * (1.f - T));
		}
	}
	if (T >= 1.f)
	{
		SetSurfaceTint(Base);
		Mesh->SetRelativeScale3D(BodyBaseScale);
		if (FxRing)
		{
			FxRing->SetVisibility(false);
		}
		FxTime = FxNone; // bitti
		return false;
	}
	return true;
}

// --- Aile ----------------------------------------------------------------------------------------

void ASLPlaceable::StartResidentsArrival(const FVector& StartPlatformLocal)
{
	if (bResidentsStarted || Residents <= 0 || !DefaultCube)
	{
		return;
	}
	bResidentsStarted = true;

	// Figur olcusu boya gore: 160'lik spec figurunun oranlari (30 cm en), bina 50 cm'e inince kucultuldu.
	const float H = ResidentHeightCm;
	const float W = FMath::Max(6.f, H * 0.19f);
	const float Gap = FMath::Max(10.f, W * 2.f);

	// Baslangic noktasi bina yereline: bina tasinsa da aile onunla gider.
	const FTransform Rel = GetRootComponent()->GetRelativeTransform();
	const FVector Start = Rel.InverseTransformPosition(StartPlatformLocal);
	// Hedef: kapinin (govdenin yerel +X yuzu) onu. Kapisiz binada aileye en yakin yuz.
	const FVector2D Dir2 = FVector2D(Start.X, Start.Y).GetSafeNormal();
	const float HalfX = BodyHalfX(), HalfY = BodyHalfY();
	const FVector Door = bHasEntrance ? FVector(HalfX + Gap, 0.f, 0.f)
		: FMath::Abs(Dir2.X) >= FMath::Abs(Dir2.Y)
			? FVector(FMath::Sign(Dir2.X) * (HalfX + Gap), 0.f, 0.f)
			: FVector(0.f, FMath::Sign(Dir2.Y) * (HalfY + Gap), 0.f);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Door.GetSafeNormal());

	// Yol: kapi aileye donukse dosdogru; degilse once yan sokaga (plakanin yanindan), sonra kapinin
	// onune, sonra kapiya. Boylece aile binanin icinden yurumez. (Kapisiz binada hep dosdogru.)
	ResidentPath.Reset();
	const float PlateHalfX = 0.5f * PlateSizeX(), PlateHalfY = 0.5f * PlateSizeY();
	if (bHasEntrance && Start.X < PlateHalfX + Gap)
	{
		const float S = Start.Y >= 0.f ? 1.f : -1.f;
		const float Lane = S * FMath::Max(FMath::Abs(Start.Y), PlateHalfY + Gap);
		ResidentPath.Add(FVector(Start.X, Lane, 0.f));
		ResidentPath.Add(FVector(PlateHalfX + Gap, Lane, 0.f));
	}
	ResidentPath.Add(Door);

	// Figurlere evin MID'i verilirse motor yeni ornek acmiyor, ayni MID'i paylastiriyor: figuru boyamak
	// evi de boyuyordu (ekranda goruldu, ev mavi). Temel materyalden ayri ornek ac.
	UMaterialInterface* BaseMat = Mesh->GetMaterial(0);
	if (const UMaterialInstanceDynamic* HouseMID = Cast<UMaterialInstanceDynamic>(BaseMat))
	{
		BaseMat = HouseMID->Parent;
	}

	for (int32 I = 0; I < Residents; ++I)
	{
		const float MH = I < 2 ? H : H * 0.7f; // ilk ikisi yetiskin, digerleri cocuk
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(DefaultCube);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(GetRootComponent());
		C->RegisterComponent();
		C->SetRelativeScale3D(FVector(W / 100.f, W / 100.f, MH / 100.f));
		const float Spread = (I - 0.5f * (Residents - 1)) * W * 1.6f;
		C->SetRelativeLocation(Start + Side * Spread + FVector(0.f, 0.f, 0.5f * MH));
		if (UMaterialInstanceDynamic* RMID = UMaterialInstanceDynamic::Create(BaseMat, C))
		{
			RMID->SetVectorParameterValue(TEXT("Tint"), ResidentTint * (0.8f + 0.2f * I));
			C->SetMaterial(0, RMID);
		}
		ResidentMeshes.Add(C);
		ResidentOffsets.Add(Side * Spread + FVector(0.f, 0.f, 0.5f * MH));
		ResidentStep.Add(0);
	}
	SetActorTickEnabled(true);
	UE_LOG(LogSLPlaceable, Log, TEXT("SL_FAMILY %s: %d kisi yola cikti, kapi %s, %d durak"), *TypeId.ToString(), Residents, *Door.ToString(), ResidentPath.Num());
}

bool ASLPlaceable::TickResidents(float DeltaSeconds)
{
	if (!bResidentsStarted || bResidentsArrived)
	{
		return false;
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
		return false;
	}
	return true;
}

void ASLPlaceable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bWalking = TickResidents(DeltaSeconds);
	const bool bFx = TickFx(DeltaSeconds);
	if (!bWalking && !bFx)
	{
		SetActorTickEnabled(false);
	}
}
