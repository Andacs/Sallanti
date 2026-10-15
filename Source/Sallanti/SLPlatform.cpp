#include "SLPlatform.h"

#include "SLGameMode.h"
#include "SLPlaceable.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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

	Tiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Tiles"));
	Tiles->SetupAttachment(TiltPivot);
	Tiles->SetMobility(EComponentMobility::Movable);
	Tiles->SetCollisionEnabled(ECollisionEnabled::NoCollision); // isin Deck'e dussun
	Tiles->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		Tiles->SetStaticMesh(CubeMesh.Object);
	}
	if (CardboardMat.Succeeded())
	{
		Tiles->SetMaterial(0, CardboardMat.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	auto MakeIsm = [&](const TCHAR* Name, UStaticMesh* M)
	{
		UInstancedStaticMeshComponent* I = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		I->SetupAttachment(TiltPivot);
		I->SetMobility(EComponentMobility::Movable);
		I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		I->SetStaticMesh(M);
		if (CardboardMat.Succeeded())
		{
			I->SetMaterial(0, CardboardMat.Object);
		}
		return I;
	};
	ArcadeLight = MakeIsm(TEXT("ArcadeLight"), CubeMesh.Object);
	ArcadeDark = MakeIsm(TEXT("ArcadeDark"), CubeMesh.Object);
	ArcadeArches = MakeIsm(TEXT("ArcadeArches"), CylMesh.Object);
	ZoneTiles = MakeIsm(TEXT("ZoneTiles"), CubeMesh.Object);
	ZoneTiles->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GhostMat(TEXT("/Game/Sallanti/Materials/M_Ghost.M_Ghost"));
	if (GhostMat.Succeeded())
	{
		ZoneTiles->SetMaterial(0, GhostMat.Object);
	}
	ApplyDeckSize();
}

void ASLPlatform::SetZoneCells(const TArray<FIntPoint>& Cells, const FLinearColor& Color, float Opacity)
{
	uint32 Sig = HashCombine(GetTypeHash(Cells.Num()), GetTypeHash(Color.ToFColor(false).DWColor()));
	for (const FIntPoint& C : Cells)
	{
		Sig = HashCombine(Sig, GetTypeHash(C));
	}
	if (Sig == ZoneSignature)
	{
		return; // her kare cagrilir; degismediyse ISM'i yeniden kurma
	}
	ZoneSignature = Sig;
	ZoneTiles->ClearInstances();
	const float S = FMath::Max(1.f, GridSize - TileGap) / 100.f;
	for (const FIntPoint& C : Cells)
	{
		// Karonun (0.6) hemen ustu, plakanin (2) ve yolun (2.5) alti: binalarin altinda kalir.
		ZoneTiles->AddInstance(FTransform(FQuat::Identity, FVector(CellToLocal(C.X + 0.5f), CellToLocal(C.Y + 0.5f), 0.9f), FVector(S, S, 0.4f / 100.f)));
	}
	if (!ZoneMID)
	{
		ZoneMID = ZoneTiles->CreateDynamicMaterialInstance(0);
	}
	if (ZoneMID)
	{
		ZoneMID->SetVectorParameterValue(TEXT("Color"), Color);
		ZoneMID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
}

TSet<FName> ASLPlatform::TypesOnRoadNetwork(const TArray<FIntPoint>& StartCells, const ASLPlaceable* Ignore, TArray<FIntPoint>* OutRoads) const
{
	auto RoadAt = [&](const FIntPoint& C) -> bool
	{
		const ASLPlaceable* R = FindPlaceableAt(C);
		return R && R != Ignore && R->bFlat;
	};
	TArray<FIntPoint> Roads;
	TSet<FIntPoint> Seen;
	for (const FIntPoint& S : StartCells)
	{
		if (RoadAt(S) && !Seen.Contains(S))
		{
			Seen.Add(S);
			Roads.Add(S);
		}
	}
	const FIntPoint Dirs[4] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
	for (int32 Q = 0; Q < Roads.Num() && Q < 2000; ++Q)
	{
		for (const FIntPoint& D : Dirs)
		{
			const FIntPoint N = Roads[Q] + D;
			if (!Seen.Contains(N) && RoadAt(N))
			{
				Seen.Add(N);
				Roads.Add(N);
			}
		}
	}
	TSet<FName> Types;
	if (Roads.Num() == 0)
	{
		if (OutRoads)
		{
			OutRoads->Reset();
		}
		return Types;
	}
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		const ASLPlaceable* B = Weak.Get();
		if (!B || B->bHeld || B == Ignore || B->bFlat)
		{
			continue;
		}
		bool bTouch = false;
		if (B->bHasEntrance)
		{
			for (const FIntPoint& E : B->GetEntranceCells())
			{
				bTouch |= Seen.Contains(E);
			}
		}
		else
		{
			const FIntPoint Mn = B->CellMin, F = B->GetFootprint();
			for (int32 X = Mn.X; X < Mn.X + F.X && !bTouch; ++X)
			{
				bTouch = Seen.Contains(FIntPoint(X, Mn.Y - 1)) || Seen.Contains(FIntPoint(X, Mn.Y + F.Y));
			}
			for (int32 Y = Mn.Y; Y < Mn.Y + F.Y && !bTouch; ++Y)
			{
				bTouch = Seen.Contains(FIntPoint(Mn.X - 1, Y)) || Seen.Contains(FIntPoint(Mn.X + F.X, Y));
			}
		}
		if (bTouch)
		{
			Types.Add(B->TypeId);
		}
	}
	if (OutRoads)
	{
		*OutRoads = MoveTemp(Roads);
	}
	return Types;
}

void ASLPlatform::BuildArcade()
{
	// Kemerli kaide: icte golgeli bir blok, kenarlarda sutunlar, ustte sutunlari baglayan bant. Kemerin
	// yuvarlak tepesi: bandin alt kenarina, disari bakan golge renkli bir disk (ust yarisi bandi oyar gibi).
	ArcadeLight->ClearInstances();
	ArcadeDark->ClearInstances();
	ArcadeArches->ClearInstances();
	if (!bArcade)
	{
		return;
	}
	const float Top = -DeckThickness;
	const float Bot = Top - ArcadeHeight;
	const float E = HalfExtent;
	const float Inset = 6.f;
	auto Box = [](UInstancedStaticMeshComponent* I, const FVector& C, const FVector& S, const FRotator& R = FRotator::ZeroRotator)
	{
		I->AddInstance(FTransform(R.Quaternion(), C, S / 100.f));
	};
	// Ic blok (aciklik arkasi) ve alt kaide (biraz tasan, kalin).
	Box(ArcadeDark, FVector(0, 0, 0.5f * (Top + Bot)), FVector(2.f * (E - Inset), 2.f * (E - Inset), ArcadeHeight));
	Box(ArcadeLight, FVector(0, 0, Bot - 9.f), FVector(2.f * E + 16.f, 2.f * E + 16.f, 18.f));
	Box(ArcadeLight, FVector(0, 0, Bot - 18.f - 40.f), FVector(1.4f * E, 1.4f * E, 80.f));
	Box(ArcadeLight, FVector(0, 0, Top - 4.f), FVector(2.f * E + 6.f, 2.f * E + 6.f, 8.f)); // tahtanin alt kornisi

	const float Bay = 2.f * E / ArcadeBays;
	const float Span = FMath::Max(4.f, Bay - PillarWidth);
	const float BandH = 0.28f * ArcadeHeight;
	const float ArchR = 0.5f * Span;
	const float ArchZ = Top - BandH;
	for (int32 Side = 0; Side < 4; ++Side)
	{
		const bool bAlongX = (Side % 2) == 0;          // 0/2: +-Y kenari (X boyunca), 1/3: +-X kenari
		const float Sign = Side < 2 ? 1.f : -1.f;
		auto At = [&](float T, float Out, float Z) { return bAlongX ? FVector(T, Sign * Out, Z) : FVector(Sign * Out, T, Z); };
		auto Sz = [&](float L, float Th, float Hh) { return bAlongX ? FVector(L, Th, Hh) : FVector(Th, L, Hh); };
		// Ust bant
		Box(ArcadeLight, At(0, E - 0.5f * Inset, Top - 0.5f * BandH), Sz(2.f * E, Inset, BandH));
		// Sutunlar (koseler dahil)
		for (int32 I = 0; I <= ArcadeBays; ++I)
		{
			const float T = -E + I * Bay;
			Box(ArcadeLight, At(T, E - 0.5f * Inset, 0.5f * (Top + Bot)), Sz(PillarWidth, Inset + 2.f, ArcadeHeight));
		}
		// Kemer tepeleri: silindir ekseni disari (yerel Z -> kenar normali), ince disk.
		const FRotator R = bAlongX ? FRotator(0, 0, 90.f) : FRotator(90.f, 0, 0);
		for (int32 I = 0; I < ArcadeBays; ++I)
		{
			const float T = -E + (I + 0.5f) * Bay;
			Box(ArcadeArches, At(T, E + 0.6f, ArchZ), FVector(2.f * ArchR, 2.f * ArchR, 1.f), R);
		}
	}
	if (UMaterialInstanceDynamic* L = ArcadeLight->CreateDynamicMaterialInstance(0))
	{
		L->SetVectorParameterValue(TEXT("Tint"), ArcadeColor);
	}
	for (UInstancedStaticMeshComponent* D : { ArcadeDark.Get(), ArcadeArches.Get() })
	{
		if (UMaterialInstanceDynamic* M = D->CreateDynamicMaterialInstance(0))
		{
			M->SetVectorParameterValue(TEXT("Tint"), ArcadeShadeColor);
		}
	}
}

void ASLPlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDeckSize();
	BuildGrid();
	BuildArcade();
}

void ASLPlatform::BuildGrid()
{
	// Her hucreye bir karo: GridSize - TileGap kare, 0.6 cm kalin, yuzeyin hemen ustunde.
	// Plaka (2 cm) ve yol (2.5 cm) bunun ustune biner; ortulen karo gorunmez.
	Tiles->ClearInstances();
	const int32 N = GetCellCount();
	const float S = FMath::Max(1.f, GridSize - TileGap) / 100.f;
	for (int32 X = 0; X < N; ++X)
	{
		for (int32 Y = 0; Y < N; ++Y)
		{
			Tiles->AddInstance(FTransform(FQuat::Identity,
				FVector(CellToLocal(X + 0.5f), CellToLocal(Y + 0.5f), 0.3f), FVector(S, S, 0.6f / 100.f)));
		}
	}
	TileMID = Tiles->CreateDynamicMaterialInstance(0);
	if (TileMID)
	{
		TileMID->SetVectorParameterValue(TEXT("Tint"), TileColor);
	}
}

uint32 ASLPlatform::ComputeSignature() const
{
	uint32 H = 17;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		const ASLPlaceable* P = Weak.Get();
		if (!P)
		{
			continue;
		}
		H = HashCombine(H, GetTypeHash(P->CellMin));
		H = HashCombine(H, GetTypeHash(P->Facing * 2 + (P->bHeld ? 1 : 0)));
		H = HashCombine(H, GetTypeHash(P->TypeId));
	}
	return H;
}

void ASLPlatform::RefreshConnections()
{
	// Kapi onu hucresi -> o kapinin binasina dogru yon. Yol karosu o yone uzanir: bina ile birlesir.
	TMap<FIntPoint, FIntPoint> DoorDirAt;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		const ASLPlaceable* B = Weak.Get();
		if (B && !B->bHeld && B->bHasEntrance)
		{
			for (const FIntPoint& E : B->GetEntranceCells())
			{
				DoorDirAt.Add(E, FIntPoint(-B->GetFacingDir().X, -B->GetFacingDir().Y));
			}
		}
	}
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		ASLPlaceable* R = Weak.Get();
		if (!R || !R->bFlat)
		{
			continue;
		}
		if (R->bHeld)
		{
			R->SetRoadLinks(0);
			continue;
		}
		// Bitler: 0 +X, 1 +Y, 2 -X, 3 -Y (platform hucre ekseni).
		const FIntPoint Dirs[4] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
		uint8 Mask = 0;
		for (int32 I = 0; I < 4; ++I)
		{
			const ASLPlaceable* N = FindPlaceableAt(R->CellMin + Dirs[I]);
			if (N && N->bFlat)
			{
				Mask |= 1 << I;
			}
		}
		if (const FIntPoint* ToHouse = DoorDirAt.Find(R->CellMin))
		{
			for (int32 I = 0; I < 4; ++I)
			{
				if (Dirs[I] == *ToHouse)
				{
					Mask |= 1 << I;
				}
			}
		}
		R->SetRoadLinks(Mask);
	}
	ConnectionSignature = ComputeSignature();
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
		if (!P || P->bHeld)
		{
			continue; // geri alinmis ya da oyuncunun elinde: dengeye katilmaz
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
		if (!P || P->bHeld || P->GetAttachParentActor() != this)
		{
			continue;
		}
		const FIntPoint OMin = P->CellMin;
		const FIntPoint OSize = P->GetFootprint();
		const bool bOverlapX = CellMin.X < OMin.X + OSize.X && OMin.X < CellMin.X + Footprint.X;
		const bool bOverlapY = CellMin.Y < OMin.Y + OSize.Y && OMin.Y < CellMin.Y + Footprint.Y;
		if (bOverlapX && bOverlapY)
		{
			Level = FMath::Max(Level, P->Level + P->GetStackHeight());
		}
	}
	return Level;
}

bool ASLPlatform::IsAreaFree(FIntPoint CellMin, FIntPoint Footprint, const ASLPlaceable* Ignore) const
{
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		const ASLPlaceable* P = Weak.Get();
		if (!P || P == Ignore || P->bHeld)
		{
			continue;
		}
		const FIntPoint OMin = P->CellMin;
		const FIntPoint OSize = P->GetFootprint();
		if (CellMin.X < OMin.X + OSize.X && OMin.X < CellMin.X + Footprint.X
			&& CellMin.Y < OMin.Y + OSize.Y && OMin.Y < CellMin.Y + Footprint.Y)
		{
			return false;
		}
	}
	return true;
}

ASLPlaceable* ASLPlatform::FindPlaceableAt(const FIntPoint& Cell) const
{
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : Placeables)
	{
		ASLPlaceable* P = Weak.Get();
		if (P && !P->bHeld && P->CoversCell(Cell))
		{
			return P;
		}
	}
	return nullptr;
}

float ASLPlatform::PredictTiltDegrees(float ExtraMass, const FVector2D& ExtraLocal) const
{
	FVector2D Weighted = CenterOfMass * (TotalMass + PlatformMass); // = Sum(m*r)
	Weighted += ExtraMass * ExtraLocal;
	const float Denominator = TotalMass + ExtraMass + PlatformMass;
	const FVector2D Com = Denominator > KINDA_SMALL_NUMBER ? Weighted / Denominator : FVector2D::ZeroVector;
	return FMath::Min(Com.Size() * TiltPerCm, MaxTilt);
}

FString ASLPlatform::DescribeLeanSide(float CameraYawDeg) const
{
	if (CenterOfMass.SizeSquared() < 1.f)
	{
		return FString();
	}
	// Yerel CoM yonu -> dunya (platform kokunun yaw'i dahil) -> kamera eksenlerine izdusum.
	const FVector World = GetActorTransform().TransformVectorNoScale(FVector(CenterOfMass.X, CenterOfMass.Y, 0.f));
	const float Yaw = FMath::DegreesToRadians(CameraYawDeg);
	const FVector2D Fwd(FMath::Cos(Yaw), FMath::Sin(Yaw));   // ekranin "ileri"si (arka taraf)
	const FVector2D Right(-FMath::Sin(Yaw), FMath::Cos(Yaw)); // ekranin sagi
	const float F = World.X * Fwd.X + World.Y * Fwd.Y;
	const float R = World.X * Right.X + World.Y * Right.Y;
	if (FMath::Abs(R) >= FMath::Abs(F))
	{
		return R > 0.f ? TEXT("sağa") : TEXT("sola");
	}
	return F > 0.f ? TEXT("arkaya") : TEXT("öne");
}

float ASLPlatform::GetWorldZOfLocal(const FVector& Local) const
{
	return TiltPivot->GetComponentTransform().TransformPosition(Local).Z;
}

void ASLPlatform::BeginPlay()
{
	Super::BeginPlay();
	DeckMID = Deck->CreateDynamicMaterialInstance(0);
	if (DeckMID)
	{
		DeckMID->SetVectorParameterValue(TEXT("Tint"), DeckTint);
	}
	if (!TileMID)
	{
		BuildGrid();
	}
	BuildArcade();
}

void ASLPlatform::ApplyTiltRotation(const FVector2D& TiltDeg)
{
	// d = inen taraf. k = Z x d ekseni etrafinda pozitif aci d tarafini asagi indirir (SLReport ile olculdu).
	const float Deg = TiltDeg.Size();
	FQuat Rot = FQuat::Identity;
	if (Deg > KINDA_SMALL_NUMBER)
	{
		const FVector Down(TiltDeg.X / Deg, TiltDeg.Y / Deg, 0.f);
		const FVector Axis = FVector::CrossProduct(FVector::UpVector, Down).GetSafeNormal();
		Rot = FQuat(Axis, FMath::DegreesToRadians(Deg));
	}
	TiltPivot->SetRelativeRotation(Rot);
}

void ASLPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (BalanceState == ESLBalanceState::Collapsed)
	{
		// Devrilme: ayni yone, hizlanarak, CollapseAngle'a kadar. Binalar bagli, birlikte gider.
		CollapseVelocity += CollapseAcceleration * DeltaSeconds;
		CollapseCurrent = FMath::Min(CollapseCurrent + CollapseVelocity * DeltaSeconds, CollapseAngle);
		CurrentTiltDegrees = CollapseCurrent;
		ApplyTiltRotation(CollapseDir * CollapseCurrent);
		UpdateDeckWarning(DeltaSeconds);
		return;
	}

	RecomputeCenterOfMass();
	if (ComputeSignature() != ConnectionSignature)
	{
		RefreshConnections(); // yol konuldu/kaldirildi/tasindi ya da bina dondu
	}

	// Hedef egim: CoM'un oldugu taraf iner. Boy = |CoM| * TiltPerCm, MaxTilt ile kirpilir.
	const float ComDist = CenterOfMass.Size();
	const FVector2D TargetTilt = ComDist > KINDA_SMALL_NUMBER
		? (CenterOfMass / ComDist) * FMath::Min(ComDist * TiltPerCm, MaxTilt)
		: FVector2D::ZeroVector;
	TargetTiltDegrees = TargetTilt.Size();
	CurrentTilt = FMath::Vector2DInterpConstantTo(CurrentTilt, TargetTilt, DeltaSeconds, TiltSpeed);

	SwayTime += DeltaSeconds;
	const float SwayRad = FMath::DegreesToRadians(SwayDirection);
	const FVector2D SwayDir(FMath::Cos(SwayRad), FMath::Sin(SwayRad));
	const FVector2D Sway = SwayDir * (FMath::Sin(SwayTime * UE_TWO_PI / SwayPeriod) * SwayAmplitude);

	const FVector2D Total = CurrentTilt + Sway;
	CurrentTiltDegrees = Total.Size();
	ApplyTiltRotation(Total);

	UpdateBalanceState(DeltaSeconds);
	UpdateDeckWarning(DeltaSeconds);
	UpdateCreak();

#if !UE_BUILD_SHIPPING
	if (bDebugDraw)
	{
		const FVector ComWorld = TiltPivot->GetComponentTransform().TransformPosition(FVector(CenterOfMass.X, CenterOfMass.Y, 2.f));
		// Sayilar ASLHUD test panelinde; burada sadece CoM'un platformdaki yeri.
		DrawDebugSphere(GetWorld(), ComWorld, 12.f, 8, FColor::Magenta, false, -1.f, SDPG_Foreground);
	}
#endif
}

void ASLPlatform::UpdateBalanceState(float DeltaSeconds)
{
	const float Judged = CurrentTilt.Size(); // sallanma haric
	WarnAlpha = FMath::Clamp((Judged - WarnTilt) / FMath::Max(FailTilt - WarnTilt, 0.01f), 0.f, 1.f);

	if (Judged >= FailTilt)
	{
		FailTimer += DeltaSeconds;
		BalanceState = ESLBalanceState::Failing;
		if (FailTimer >= FailSeconds)
		{
			Collapse();
		}
		return;
	}
	FailTimer = 0.f; // kesintisiz olmali: esigin altina inen sayaci sifirlar
	BalanceState = Judged >= WarnTilt ? ESLBalanceState::Warning : ESLBalanceState::Stable;
}

void ASLPlatform::UpdateDeckWarning(float DeltaSeconds)
{
	if (!DeckMID)
	{
		return;
	}
	float Strength = 0.f;
	float Hz = 0.f;
	switch (BalanceState)
	{
	case ESLBalanceState::Warning:
		Strength = FMath::Lerp(WarnStrengthMin, WarnStrengthMax, WarnAlpha);
		Hz = FMath::Lerp(WarnPulseHzMin, WarnPulseHzMax, WarnAlpha);
		break;
	case ESLBalanceState::Failing:   Strength = WarnStrengthMax; Hz = FailingPulseHz; break;
	case ESLBalanceState::Collapsed: Strength = WarnStrengthMax; Hz = 0.f; break;
	default: break;
	}
	WarnPhase += DeltaSeconds * Hz * UE_TWO_PI;
	// Nabiz yarim genlikte: en sonuk aninda bile Strength'in yarisi kadar kirmizi kalir, uyari kaybolmaz.
	const float Pulse = Hz > 0.f ? 0.75f + 0.25f * FMath::Sin(WarnPhase) : 1.f;
	DeckMID->SetVectorParameterValue(TEXT("Tint"), FMath::Lerp(DeckTint, WarnColor, Strength * Pulse));
	if (TileMID)
	{
		// Karolar da atar: tahtanin cogu karo, yalniz derzler kizarsa uyari kaybolur.
		TileMID->SetVectorParameterValue(TEXT("Tint"), FMath::Lerp(TileColor, WarnColor, Strength * Pulse));
	}
}

void ASLPlatform::UpdateCreak()
{
	const bool bWant = BalanceState == ESLBalanceState::Warning || BalanceState == ESLBalanceState::Failing;
	if (!bWant)
	{
		if (Creak && Creak->IsPlaying())
		{
			Creak->FadeOut(0.3f, 0.f);
		}
		return;
	}
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const TObjectPtr<USoundBase>* Snd = GM ? GM->Sounds.Find(TEXT("SFX_Gicirti_Dongu")) : nullptr;
	if (!Snd || !*Snd)
	{
		return;
	}
	const bool bFail = BalanceState == ESLBalanceState::Failing;
	const float Vol = (bFail ? CreakVolumeMax : FMath::Lerp(CreakVolumeMin, CreakVolumeMax, WarnAlpha)) * GM->MasterSfxVolume;
	const float Pitch = bFail ? 1.3f : FMath::Lerp(0.9f, 1.15f, WarnAlpha);
	if (!Creak)
	{
		// Sessiz baslatilan ses fade baslamadan cull ediliyor (04_UNREAL_TUZAKLARI): gercek sesle baslat.
		Creak = UGameplayStatics::SpawnSound2D(this, *Snd, Vol, Pitch, 0.f, nullptr, false, false);
	}
	if (Creak)
	{
		if (!Creak->IsPlaying())
		{
			Creak->SetVolumeMultiplier(Vol);
			Creak->Play();
		}
		Creak->SetVolumeMultiplier(Vol);
		Creak->SetPitchMultiplier(Pitch);
	}
}

void ASLPlatform::Collapse()
{
	BalanceState = ESLBalanceState::Collapsed;
	const float Deg = CurrentTilt.Size();
	CollapseDir = Deg > KINDA_SMALL_NUMBER ? CurrentTilt / Deg : FVector2D(1.f, 0.f);
	CollapseCurrent = Deg;
	CollapseVelocity = TiltSpeed * 0.5f; // eldeki egimden yumusak devam
	UE_LOG(LogTemp, Log, TEXT("SL_COLLAPSE egim %.2f deg, %.2f sn FailTilt (%.1f) ustunde, yon (%.2f, %.2f)"),
		Deg, FailTimer, FailTilt, CollapseDir.X, CollapseDir.Y);
	if (Creak)
	{
		Creak->Stop();
	}
	ASLGameMode::PlaySfx(this, TEXT("SFX_Devrilme"), 6);
	OnCollapsed.Broadcast();
}
