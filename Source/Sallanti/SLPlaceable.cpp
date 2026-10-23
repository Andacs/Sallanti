#include "SLPlaceable.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	DefaultCube = CubeMesh.Object;
	DiscMesh = CylMesh.Object;
	ConeMesh = ConeFinder.Object;
	SphereMesh = SphereFinder.Object;
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
	BuildDecor();
	BuildRoadLinks();
}

// --- Cozy suslemesi -------------------------------------------------------------------------------

void ASLPlaceable::BuildStopDecor()
{
	// Otobus duragi: govde kutusu yerine sundurma. Onu (+X) yola acik: iki direk, turkuaz cati, arkada cam,
	// bank, yanda yuvarlak "D" tabelali direk. Kutu gizli; isin plakaya duser (tiklayinca tasinir).
	Mesh->SetVisibility(false);
	const float Base = PlateHeightCm;
	const float PX = 0.5f * PlateSizeX(), PY = 0.5f * PlateSizeY();
	const float H = 30.f;
	const float Back = -PX + 6.f, Front = PX - 8.f;
	for (int32 S = -1; S <= 1; S += 2)
	{
		AddDecor(DefaultCube, FVector(Front, S * (PY - 6.f), Base + 0.5f * H), FVector(2.5f, 2.5f, H), FLinearColor(0.55f, 0.55f, 0.60f));
	}
	AddWindowPart(FVector(Back, 0, Base + 0.5f * H), FVector(2.f, 2.f * PY - 8.f, H)); // arka cam (gece yanar)
	AddDecor(DefaultCube, FVector(0.5f * (Back + Front), 0, Base + H + 1.5f), FVector(Front - Back + 8.f, 2.f * PY - 2.f, 3.f), AccentColor, FRotator(-6.f, 0, 0)); // cati
	AddDecor(DefaultCube, FVector(Back + 7.f, 0, Base + 7.f), FVector(8.f, 2.f * PY - 16.f, 2.f), FLinearColor(0.80f, 0.55f, 0.40f)); // bank
	AddDecor(DefaultCube, FVector(Back + 4.f, 0, Base + 3.5f), FVector(1.5f, 2.f * PY - 18.f, 7.f), FLinearColor(0.55f, 0.55f, 0.60f));
	// Tabela: direk + disk (eksen +X) + "D"
	const FVector Pole(Front, PY - 1.f, Base);
	AddDecor(DefaultCube, Pole + FVector(0, 0, 22.f), FVector(1.6f, 1.6f, 44.f), FLinearColor(0.55f, 0.55f, 0.60f));
	AddDecor(DiscMesh, Pole + FVector(0.8f, 0, 44.f), FVector(14.f, 14.f, 1.2f), RoofColor, FRotator(90.f, 0, 0));
	UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
	Text->SetupAttachment(GetRootComponent());
	Text->RegisterComponent();
	Text->SetText(FText::FromString(TEXT("D")));
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(10.f);
	Text->SetTextRenderColor(FColor::White);
	Text->SetRelativeLocation(Pole + FVector(1.6f, 0, 44.f));
	Text->SetCastShadow(false);
	DecorParts.Add(Text);
}

void ASLPlaceable::AddWindowPart(const FVector& Loc, const FVector& Size)
{
	// Isiksiz yari saydam malzeme (M_Ghost, opaklik 1): gece sicak sariya donunce "yanar".
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(DefaultCube);
	C->SetMobility(EComponentMobility::Movable);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetupAttachment(GetRootComponent());
	C->RegisterComponent();
	C->SetRelativeScale3D(Size / 100.f);
	C->SetRelativeLocation(Loc);
	if (UMaterialInstanceDynamic* W = FxMaterial ? UMaterialInstanceDynamic::Create(FxMaterial, C) : nullptr)
	{
		W->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		C->SetMaterial(0, W);
		WindowMIDs.Add(W);
	}
	DecorParts.Add(C);
	SetNightGlow(NightGlow);
}

void ASLPlaceable::SetNightGlow(float Night)
{
	NightGlow = FMath::Clamp(Night, 0.f, 1.f);
	const FLinearColor C = FMath::Lerp(WindowColor * WindowDayBrightness, WindowNightColor * WindowNightBrightness, NightGlow);
	for (UMaterialInstanceDynamic* W : WindowMIDs)
	{
		if (W)
		{
			W->SetVectorParameterValue(TEXT("Color"), C);
		}
	}
}

void ASLPlaceable::BuildParkDecor()
{
	// Park 2x2: cimen plaka, capraz krem patika, kosede gol, yuvarlak agaclar + servi, bank, cicekler.
	Mesh->SetVisibility(false);
	PlateColor = FLinearColor(0.52f, 0.80f, 0.50f);
	if (PlateMID)
	{
		PlateMID->SetVectorParameterValue(TEXT("Tint"), PlateColor);
	}
	const float Base = PlateHeightCm;
	const float PX = 0.5f * PlateSizeX(), PY = 0.5f * PlateSizeY();
	const FLinearColor Path(0.97f, 0.90f, 0.78f);
	AddDecor(DefaultCube, FVector(0, 0, Base + 0.3f), FVector(2.f * PX - 10.f, 10.f, 0.6f), Path);
	AddDecor(DefaultCube, FVector(0, 0, Base + 0.35f), FVector(10.f, 2.f * PY - 10.f, 0.7f), Path);
	AddDecor(DiscMesh, FVector(0.45f * PX, 0.45f * PY, Base + 0.4f), FVector(0.65f * PX, 0.55f * PY, 0.8f), FLinearColor(0.45f, 0.70f, 0.95f)); // gol
	auto Round = [&](const FVector& At, float H)
	{
		AddDecor(DefaultCube, At + FVector(0, 0, 0.3f * H), FVector(2.5f, 2.5f, 0.6f * H), FLinearColor(0.50f, 0.36f, 0.26f));
		AddDecor(SphereMesh, At + FVector(0, 0, 0.75f * H), FVector(0.55f * H, 0.55f * H, 0.5f * H), BushColor * 0.9f);
	};
	Round(FVector(-0.55f * PX, -0.55f * PY, Base), 26.f);
	Round(FVector(-0.55f * PX, 0.5f * PY, Base), 22.f);
	AddCypress(FVector(0.55f * PX, -0.6f * PY, Base), 34.f);
	AddCypress(FVector(0.7f * PX, -0.25f * PY, Base), 26.f);
	AddDecor(DefaultCube, FVector(-0.1f * PX, -0.35f * PY, Base + 4.f), FVector(16.f, 5.f, 1.5f), FLinearColor(0.80f, 0.55f, 0.40f)); // bank
	AddDecor(DefaultCube, FVector(-0.1f * PX, -0.35f * PY - 2.5f, Base + 6.5f), FVector(16.f, 1.2f, 4.f), FLinearColor(0.80f, 0.55f, 0.40f));
	const FLinearColor Flowers[3] = { FLinearColor(0.98f, 0.55f, 0.60f), FLinearColor(1.f, 0.85f, 0.35f), FLinearColor(0.80f, 0.60f, 0.95f) };
	for (int32 I = 0; I < 9; ++I)
	{
		const float A = I * 2.4f;
		const FVector At(-0.35f * PX + 0.25f * PX * FMath::Cos(A), 0.2f * PY + 0.25f * PY * FMath::Sin(A), Base + 1.5f);
		AddDecor(SphereMesh, At, FVector(3.f, 3.f, 3.f), Flowers[I % 3]);
	}
}

void ASLPlaceable::ClearDecor()
{
	WindowMIDs.Reset();
	for (USceneComponent* C : DecorParts)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	DecorParts.Reset();
}

UStaticMeshComponent* ASLPlaceable::AddDecor(UStaticMesh* InMesh, const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color, const FRotator& Rot)
{
	UStaticMeshComponent* C = MakePart(Color, false, InMesh);
	C->SetCastShadow(true);
	C->SetRelativeRotation(Rot);
	C->SetRelativeScale3D(SizeCm / 100.f); // motorun hazir sekilleri 100 cm, merkez pivotlu
	C->SetRelativeLocation(Loc);
	DecorParts.Add(C);
	return C;
}

void ASLPlaceable::AddCypress(const FVector& Base, float Height)
{
	const float W = Height * 0.34f;
	const float Trunk = Height * 0.14f;
	AddDecor(DefaultCube, Base + FVector(0, 0, 0.5f * Trunk), FVector(W * 0.22f, W * 0.22f, Trunk), FLinearColor(0.50f, 0.36f, 0.26f));
	const float Crown = Height - Trunk;
	// Servi: koni + tabanda basik kure, yumusak "damla" silueti.
	AddDecor(ConeMesh, Base + FVector(0, 0, Trunk + 0.55f * Crown), FVector(W, W, Crown * 0.9f), TreeColor);
	AddDecor(SphereMesh, Base + FVector(0, 0, Trunk + 0.2f * Crown), FVector(W, W, Crown * 0.4f), TreeColor);
}

FVector ASLPlaceable::FaceLoc(int32 Face, float Along, float Out, float Z) const
{
	switch (Face & 3)
	{
	case 0: return FVector(Out, Along, Z);
	case 1: return FVector(Along, Out, Z);
	case 2: return FVector(-Out, Along, Z);
	default: return FVector(Along, -Out, Z);
	}
}

FVector ASLPlaceable::FaceSize(int32 Face, float AlongSize, float OutSize, float H) const
{
	return (Face & 1) ? FVector(AlongSize, OutSize, H) : FVector(OutSize, AlongSize, H);
}

float ASLPlaceable::FaceDepth(int32 Face) const
{
	return (Face & 1) ? BodyHalfY() : BodyHalfX();
}

float ASLPlaceable::FaceHalfLen(int32 Face) const
{
	return (Face & 1) ? BodyHalfX() : BodyHalfY();
}

void ASLPlaceable::AddWindowGrid(int32 Face, float Bottom, float Top, int32 Rows, int32 Cols, float WinW, float WinH, bool bSkipDoorColumn,
	bool bFlowers, bool bShutters)
{
	// Yuzler: 0 +X (kapi), 1 +Y, 2 -X, 3 -Y. Cicek kutusu yalnizca alt sirada.
	const float Half = FaceHalfLen(Face);
	for (int32 R = 0; R < Rows; ++R)
	{
		const float Z = Rows == 1 ? 0.5f * (Bottom + Top) : FMath::Lerp(Bottom, Top, float(R) / (Rows - 1));
		for (int32 C = 0; C < Cols; ++C)
		{
			const float T = Cols == 1 ? 0.f : FMath::Lerp(-Half * 0.62f, Half * 0.62f, float(C) / (Cols - 1));
			if (bSkipDoorColumn && R == 0 && FMath::Abs(T) < WinW)
			{
				continue;
			}
			AddFramedWindow(Face, T, Z, WinW, WinH, bFlowers && R == 0, bShutters);
		}
	}
}

void ASLPlaceable::AddFramedWindow(int32 Face, float Along, float Z, float W, float H, bool bFlowers, bool bShutters)
{
	// Katmanlar govdeden disari: cerceve (D-0.3..D+0.9) < cam (D-0.35..D+1.15) < kayit (D+1.05..D+1.55).
	const float D = FaceDepth(Face);
	const float B = FMath::Clamp(0.18f * FMath::Min(W, H), 1.2f, 3.5f); // cerceve payi
	AddDecor(DefaultCube, FaceLoc(Face, Along, D + 0.3f, Z), FaceSize(Face, W + 2.f * B, 1.2f, H + 2.f * B), TrimColor);
	AddWindowPart(FaceLoc(Face, Along, D + 0.4f, Z), FaceSize(Face, W, 1.5f, H));
	const float Bar = FMath::Max(0.6f, 0.08f * FMath::Min(W, H));
	AddDecor(DefaultCube, FaceLoc(Face, Along, D + 1.3f, Z), FaceSize(Face, Bar, 0.5f, H), TrimColor);
	AddDecor(DefaultCube, FaceLoc(Face, Along, D + 1.3f, Z), FaceSize(Face, W, 0.5f, Bar), TrimColor);
	const float Bottom = Z - 0.5f * H - B;
	if (bFlowers)
	{
		// Cicek kutusu: toprak rengi kutu, ustunde yesil kume ve renkli ciceker.
		const float BoxW = W + 2.f * B, BoxH = FMath::Clamp(0.22f * H, 1.8f, 4.f), BoxD = FMath::Clamp(0.3f * W, 2.5f, 5.f);
		const float BoxZ = Bottom - 0.5f * BoxH;
		AddDecor(DefaultCube, FaceLoc(Face, Along, D + 0.5f * BoxD, BoxZ), FaceSize(Face, BoxW, BoxD, BoxH), PlanterColor);
		AddDecor(SphereMesh, FaceLoc(Face, Along, D + 0.5f * BoxD, BoxZ + 0.5f * BoxH), FaceSize(Face, BoxW - 0.5f, BoxD * 0.9f, BoxH * 1.1f), BushColor);
		const int32 N = FMath::Clamp(FMath::RoundToInt(BoxW / 4.f), 2, 7);
		const float Dot = FMath::Clamp(BoxW / N * 0.75f, 1.6f, 3.5f);
		for (int32 I = 0; I < N && FlowerColors.Num() > 0; ++I)
		{
			const float A = Along - 0.5f * BoxW + (I + 0.5f) * BoxW / N;
			AddDecor(SphereMesh, FaceLoc(Face, A, D + 0.6f * BoxD, BoxZ + 0.5f * BoxH + 0.35f * Dot), FVector(Dot), FlowerColors[I % FlowerColors.Num()]);
		}
	}
	else
	{
		AddDecor(DefaultCube, FaceLoc(Face, Along, D + 1.f, Bottom - 0.3f * B), FaceSize(Face, W + 2.f * B + 1.5f, 2.4f, FMath::Max(1.f, 0.6f * B)), TrimColor);
	}
	if (bShutters)
	{
		const float SW = 0.42f * W;
		for (int32 S = -1; S <= 1; S += 2)
		{
			AddDecor(DefaultCube, FaceLoc(Face, Along + S * (0.5f * W + B + 0.5f * SW), D + 0.5f, Z), FaceSize(Face, SW, 1.f, H + B), ShutterColor);
		}
	}
}

void ASLPlaceable::AddAwning(int32 Face, float Along, float ZTop, float Width, float Depth, const FLinearColor& Color)
{
	// Seritler duvardan one egik iner; on kenarda her seridin altinda yuvarlak firfir.
	const float D = FaceDepth(Face);
	const int32 N = FMath::Max(2, AwningStripes);
	const float W = Width / N;
	const float Pitch = 22.f;
	const float C = FMath::Cos(FMath::DegreesToRadians(Pitch)), S = FMath::Sin(FMath::DegreesToRadians(Pitch));
	for (int32 I = 0; I < N; ++I)
	{
		const FLinearColor Stripe = (I % 2) ? TrimColor : Color;
		const float A = Along - 0.5f * Width + (I + 0.5f) * W;
		AddDecor(DefaultCube, FaceLoc(Face, A, D + 0.5f * Depth * C, ZTop - 0.5f * Depth * S), FVector(Depth, W + 0.05f, 1.2f), Stripe,
			FRotator(-Pitch, 90.f * (Face & 3), 0.f));
		AddDecor(SphereMesh, FaceLoc(Face, A, D + Depth * C, ZTop - Depth * S - 0.15f * W), FaceSize(Face, 0.95f * W, 1.4f, 0.6f * W), Stripe);
	}
	AddDecor(DefaultCube, FaceLoc(Face, Along, D + 0.6f, ZTop + 0.6f), FaceSize(Face, Width + 2.f, 1.2f, 1.5f), TrimColor);
}

void ASLPlaceable::AddPlanter(const FVector& Center, const FVector& Size, int32 Flowers)
{
	AddDecor(DefaultCube, Center + FVector(0, 0, 0.5f * Size.Z), Size, PlanterColor);
	AddDecor(SphereMesh, Center + FVector(0, 0, Size.Z), FVector(Size.X - 0.5f, Size.Y - 0.5f, Size.Z * 0.9f), BushColor);
	const bool bAlongX = Size.X >= Size.Y;
	const float Len = bAlongX ? Size.X : Size.Y;
	const float Dot = FMath::Clamp(Len / FMath::Max(1, Flowers) * 0.7f, 1.6f, 3.5f);
	for (int32 I = 0; I < Flowers && FlowerColors.Num() > 0; ++I)
	{
		const float T = -0.5f * Len + (I + 0.5f) * Len / Flowers;
		const FVector At = Center + (bAlongX ? FVector(T, 0, 0) : FVector(0, T, 0)) + FVector(0, 0, Size.Z * 1.3f);
		AddDecor(SphereMesh, At, FVector(Dot), FlowerColors[I % FlowerColors.Num()]);
	}
}

void ASLPlaceable::AddLamp(const FVector& Base, float Height)
{
	// Direk + fener (pencere gibi: gece sicak sari yanar) + kulah.
	AddDecor(DefaultCube, Base + FVector(0, 0, 0.5f * Height), FVector(1.4f, 1.4f, Height), LampPostColor);
	AddWindowPart(Base + FVector(0, 0, Height + 2.2f), FVector(4.f, 4.f, 4.4f));
	AddDecor(ConeMesh, Base + FVector(0, 0, Height + 5.8f), FVector(6.f, 6.f, 3.f), LampPostColor);
}

void ASLPlaceable::AddRoundTree(const FVector& Base, float Height)
{
	AddDecor(DefaultCube, Base + FVector(0, 0, 0.3f * Height), FVector(0.1f * Height, 0.1f * Height, 0.6f * Height), FLinearColor(0.50f, 0.36f, 0.26f));
	AddDecor(SphereMesh, Base + FVector(0, 0, 0.72f * Height), FVector(0.6f * Height, 0.6f * Height, 0.55f * Height), BushColor * 0.9f);
}

void ASLPlaceable::AddGableRoof(float Top, const FLinearColor& Gable, const FLinearColor& Roof)
{
	// Govde boyunca 45 derece cevrilmis kup: ust yarisi ucgen alinlik. Ustune iki cati levhasi ve mahya.
	// Mahya kapiya dik (X boyunca): alinlik kapinin ustunde.
	const float HX = BodyHalfX();
	const float W = 2.f * BodyHalfY();
	const float S = W / UE_SQRT_2;
	AddDecor(DefaultCube, FVector(0, 0, Top), FVector(2.f * HX, S, S), Gable, FRotator(0, 0, 45.f));
	const float SlabLen = 0.5f * W * UE_SQRT_2 + 4.f;
	const float SlabT = FMath::Clamp(0.06f * W, 3.f, 6.f);
	const float Over = FMath::Clamp(0.08f * HX, 3.f, 8.f); // sacak
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		const FVector Mid(0, Side * 0.25f * W, Top + 0.25f * W);
		const FVector N = FVector(0, Side, 1).GetSafeNormal();
		AddDecor(DefaultCube, Mid + N * (0.5f * SlabT) + FVector(0, Side * 1.f, -1.f), FVector(2.f * HX + 2.f * Over, SlabLen, SlabT), Roof, FRotator(0, 0, Side * 45.f));
	}
	AddDecor(DefaultCube, FVector(0, 0, Top + 0.5f * W + 0.4f * SlabT), FVector(2.f * HX + 2.f * Over + 2.f, 1.3f * SlabT, 1.3f * SlabT), Roof * 0.8f, FRotator(0, 0, 45.f));
}

void ASLPlaceable::BuildDecor()
{
	ClearDecor();
	if (!bDecor || bFlat || bGhost || DisplayMesh || !DefaultCube || !ConeMesh || !SphereMesh || !GetWorld())
	{
		return;
	}
	if (TypeId == TEXT("Durak"))
	{
		BuildStopDecor();
		return;
	}
	if (TypeId == TEXT("Park"))
	{
		BuildParkDecor();
		return;
	}
	const float H = SizeCells.Z * GridSize;
	const float Base = PlateHeightCm;
	const float PX = 0.5f * PlateSizeX(), PY = 0.5f * PlateSizeY();

	// Plakanin arka koselerinde servi, on koselerde cali (kapinin onu bos kalir). Govde altinda koyu supurgelik.
	const float Margin = 0.5f * BodyInsetCm;
	const float TreeH = FMath::Clamp(H * 0.55f, 22.f, 60.f);
	AddCypress(FVector(-PX + Margin, -PY + Margin, Base), TreeH);
	AddCypress(FVector(-PX + Margin, PY - Margin, Base), TreeH * 0.85f);
	const float BushD = FMath::Max(5.f, BodyInsetCm * 0.8f);
	AddDecor(SphereMesh, FVector(PX - Margin, PY - Margin, Base + 0.3f * BushD), FVector(BushD, BushD, BushD * 0.75f), BushColor);
	AddDecor(SphereMesh, FVector(PX - Margin, -PY + Margin, Base + 0.3f * BushD), FVector(BushD, BushD, BushD * 0.75f), BushColor);
	const float Skirt = FMath::Clamp(0.06f * H, 3.f, 7.f);
	AddDecor(DefaultCube, FVector(0, 0, Base + 0.5f * Skirt), FVector(2.f * BodyHalfX() + 1.6f, 2.f * BodyHalfY() + 1.6f, Skirt), Tint * 0.82f);

	if (TypeId == TEXT("Ev"))
	{
		BuildHouseDecor();
	}
	else if (TypeId == TEXT("Hastane"))
	{
		BuildHospitalDecor();
	}
	else if (TypeId == TEXT("AVM"))
	{
		BuildMallDecor();
	}
}

void ASLPlaceable::BuildHouseDecor()
{
	// Mercan besik cati, bacasi sapkali; alinlikta yuvarlak pencere; yanlarda panjurlu, cicek kutulu
	// pencereler; kapinin ustunde kucuk sacak, onunde basamak, yaninda fener.
	const float HX = BodyHalfX(), HY = BodyHalfY();
	const float Base = PlateHeightCm;
	const float H = SizeCells.Z * GridSize;
	const float Top = Base + H;
	const float W = 2.f * HY;
	AddGableRoof(Top, Tint, RoofColor);
	AddDecor(DefaultCube, FVector(-0.35f * HX, 0.3f * W, Top + 0.4f * W), FVector(6.f, 6.f, 0.5f * W), RoofColor * 0.85f);
	AddDecor(DefaultCube, FVector(-0.35f * HX, 0.3f * W, Top + 0.65f * W + 0.75f), FVector(8.f, 8.f, 1.5f), LampPostColor);
	AddDecor(DiscMesh, FVector(HX + 0.6f, 0, Top + 0.2f * W), FVector(0.3f * W, 0.3f * W, 1.2f), TrimColor, FRotator(90.f, 0, 0));
	AddWindowPart(FVector(HX + 1.0f, 0, Top + 0.2f * W), FVector(1.f, 0.17f * W, 0.17f * W));

	const float WinW = 0.7f * HX, WinH = 0.25f * H, WinZ = Base + 0.55f * H;
	AddWindowGrid(1, WinZ, WinZ, 1, 1, WinW, WinH, false, true, true);
	AddWindowGrid(3, WinZ, WinZ, 1, 1, WinW, WinH, false, true, true);
	AddWindowGrid(2, WinZ, WinZ, 1, 1, 0.7f * HY, WinH, false, false, true);

	const float DoorW = FMath::Clamp(W * 0.35f, 8.f, 60.f);
	const float DoorH = FMath::Min(H * 0.55f, 90.f);
	AddDecor(DefaultCube, FVector(HX + 0.5f, 0, Base + 0.5f * DoorH + 0.6f), FVector(1.f, DoorW + 2.4f, DoorH + 1.2f), TrimColor);
	AddDecor(DefaultCube, FVector(HX + 3.f, 0, Base + DoorH + 2.5f), FVector(6.f, DoorW + 5.f, 1.2f), RoofColor, FRotator(-18.f, 0, 0));
	AddDecor(DefaultCube, FVector(HX + 1.6f, 0, Base + 0.6f), FVector(3.2f, DoorW + 3.f, 1.2f), TrimColor);
	AddLamp(FVector(HX + 4.f, 0.5f * DoorW + 5.5f, Base), 0.3f * H); // -Y yaninda aile durur
}

void ASLPlaceable::BuildHospitalDecor()
{
	// Gok mavisi beyaz govde, turkuaz besik cati; alinlikta turkuaz halka icinde beyaz daire, kirmizi hac.
	// Kapinin ustunde direkli sacak ve tabela, iki yaninda fener ve cicek saksisi; cerceveli pencere siralari.
	const float HX = BodyHalfX(), HY = BodyHalfY();
	const float Base = PlateHeightCm;
	const float H = SizeCells.Z * GridSize;
	const float Top = Base + H;
	const float W = 2.f * HY;
	AddDecor(DefaultCube, FVector(0, 0, Top - 2.f), FVector(2.f * HX + 4.f, 2.f * HY + 4.f, 4.f), TrimColor); // kornis
	AddGableRoof(Top, Tint, HospitalRoofColor);
	const float BadgeZ = Top + 0.2f * W;
	AddDecor(DiscMesh, FVector(HX + 0.6f, 0, BadgeZ), FVector(0.36f * W, 0.36f * W, 1.2f), HospitalRoofColor * 0.85f, FRotator(90.f, 0, 0));
	AddDecor(DiscMesh, FVector(HX + 1.4f, 0, BadgeZ), FVector(0.29f * W, 0.29f * W, 1.2f), TrimColor, FRotator(90.f, 0, 0));
	const float CrossL = 0.2f * W, CrossT = CrossL * 0.32f;
	AddDecor(DefaultCube, FVector(HX + 2.2f, 0, BadgeZ), FVector(1.2f, CrossL, CrossT), RoofColor);
	AddDecor(DefaultCube, FVector(HX + 2.2f, 0, BadgeZ), FVector(1.2f, CrossT, CrossL), RoofColor);

	const float WinW = 0.14f * W, WinH = 0.15f * H;
	AddWindowGrid(1, Base + 0.27f * H, Base + 0.8f * H, 3, 4, WinW, WinH, false, true);
	AddWindowGrid(3, Base + 0.27f * H, Base + 0.8f * H, 3, 4, WinW, WinH, false, true);
	AddWindowGrid(2, Base + 0.27f * H, Base + 0.8f * H, 3, 3, WinW, WinH, false, true);
	AddWindowGrid(0, Base + 0.8f * H, Base + 0.8f * H, 1, 4, WinW, WinH, false);

	const float DoorW = FMath::Clamp(W * 0.35f, 8.f, 60.f);
	const float DoorH = FMath::Min(H * 0.55f, 90.f);
	AddDecor(DefaultCube, FVector(HX + 0.5f, 0, Base + 0.5f * DoorH + 1.f), FVector(1.f, DoorW + 4.f, DoorH + 2.f), TrimColor);
	const float CanopyZ = Base + DoorH + 5.f;
	AddDecor(DefaultCube, FVector(HX + 5.f, 0, CanopyZ), FVector(11.f, DoorW + 14.f, 2.5f), HospitalRoofColor, FRotator(-6.f, 0, 0));
	for (int32 S = -1; S <= 1; S += 2)
	{
		AddDecor(DefaultCube, FVector(HX + 6.f, S * (0.5f * DoorW + 5.f), Base + 0.5f * (CanopyZ - Base)), FVector(2.2f, 2.2f, CanopyZ - Base), TrimColor);
		AddLamp(FVector(HX + 5.f, S * (0.5f * DoorW + 13.f), Base), 0.2f * H);
		AddPlanter(FVector(HX + 4.f, S * (0.5f * DoorW + 26.f), Base), FVector(5.f, 14.f, 4.f), 4);
	}
	UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
	Text->SetupAttachment(GetRootComponent());
	Text->RegisterComponent();
	Text->SetText(FText::FromString(TEXT("HASTANE")));
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(0.075f * H);
	Text->SetTextRenderColor(FColor(64, 132, 138));
	Text->SetRelativeLocation(FVector(HX + 0.8f, 0, CanopyZ + 0.075f * H));
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	DecorParts.Add(Text);
}

void ASLPlaceable::BuildMallDecor()
{
	// Zemin kat: dukkan vitrinleri, her birinin ustunde baska renk cizgili tente. Kat arasinda krem silme;
	// ust katlarda cerceveli pencereler; kapinin ustunde buyuk tente ve "AVM" tabelasi. Duz catida
	// korkuluk ve kucuk bahce (agaclar, saksilar, bank); kapinin iki yaninda fener ve saksi.
	const float HX = BodyHalfX(), HY = BodyHalfY();
	const float Base = PlateHeightCm;
	const float H = SizeCells.Z * GridSize;
	const float Top = Base + H;
	const float GroundH = 0.32f * H;
	const int32 NC = FMath::Max(1, AwningColors.Num());
	int32 Shop = 0;
	auto Shopfront = [&](int32 Face, float Along, float ShopW)
	{
		AddFramedWindow(Face, Along, Base + 0.42f * GroundH, ShopW, 0.5f * GroundH, false, false);
		AddAwning(Face, Along, Base + GroundH - 3.f, ShopW + 8.f, 0.26f * GroundH, AwningColors.Num() ? AwningColors[Shop++ % NC] : AccentColor);
	};
	for (int32 F = 1; F < 4; ++F)
	{
		const float Len = 2.f * FaceHalfLen(F);
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Len / 75.f));
		const float Step = Len / Count;
		for (int32 I = 0; I < Count; ++I)
		{
			Shopfront(F, -0.5f * Len + (I + 0.5f) * Step, Step * 0.68f);
		}
	}
	const float DoorW = FMath::Clamp(2.f * HY * 0.35f, 8.f, 60.f);
	const float DoorH = FMath::Min(H * 0.55f, 90.f);
	const float SideW = FMath::Max(10.f, (HY - 0.5f * DoorW - 14.f) * 0.7f);
	for (int32 S = -1; S <= 1; S += 2)
	{
		Shopfront(0, S * 0.5f * (HY + 0.5f * DoorW + 8.f), SideW);
	}
	// Silme: kapi zemin kattan uzun, on yuzde kapinin ustunu bos birak.
	const float BandZ = Base + GroundH + 2.f;
	AddDecor(DefaultCube, FVector(-1.5f, 0, BandZ), FVector(2.f * HX + 3.f, 2.f * HY + 3.f, 4.f), TrimColor);
	const float Gap = 0.5f * DoorW + 3.f, BandLen = HY + 1.5f - Gap;
	for (int32 S = -1; S <= 1; S += 2)
	{
		AddDecor(DefaultCube, FVector(HX + 0.75f, S * (Gap + 0.5f * BandLen), BandZ), FVector(1.5f, BandLen, 4.f), TrimColor);
	}

	const float WinW = 0.09f * 2.f * FMath::Min(HX, HY), WinH = 0.11f * H;
	const float Row1 = Base + 0.55f * H, Row2 = Base + 0.69f * H; // tente ile tabela arasi
	for (int32 F = 0; F < 4; ++F)
	{
		const int32 Cols = FMath::Clamp(FMath::RoundToInt(2.f * FaceHalfLen(F) / 45.f), 3, 6);
		AddWindowGrid(F, Row1, Row2, 2, Cols, WinW, WinH, F == 0, true);
	}

	// Kapi: buyuk tente, tabela, fenerler, saksilar.
	AddDecor(DefaultCube, FVector(HX + 0.5f, 0, Base + 0.5f * DoorH + 1.f), FVector(1.f, DoorW + 4.f, DoorH + 2.f), TrimColor);
	AddAwning(0, 0.f, Base + DoorH + 4.f, DoorW + 16.f, 0.3f * GroundH, AwningColors.Num() ? AwningColors[0] : AccentColor);
	const float SignZ = Base + 0.86f * H;
	AddDecor(DefaultCube, FVector(HX + 1.5f, 0, SignZ), FVector(3.f, 0.5f * HY * 2.f, 0.14f * H), RoofColor);
	AddDecor(DefaultCube, FVector(HX + 1.2f, 0, SignZ), FVector(2.4f, 0.5f * HY * 2.f + 4.f, 0.14f * H + 4.f), TrimColor);
	UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
	Text->SetupAttachment(GetRootComponent());
	Text->RegisterComponent();
	Text->SetText(FText::FromString(TEXT("AVM")));
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(0.11f * H);
	Text->SetTextRenderColor(FColor(255, 248, 235));
	Text->SetRelativeLocation(FVector(HX + 3.2f, 0, SignZ));
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	DecorParts.Add(Text);
	for (int32 S = -1; S <= 1; S += 2)
	{
		AddLamp(FVector(HX + 5.f, S * (0.5f * DoorW + 10.f), Base), 0.17f * H);
	}

	// Cati: kornis, deck, alcak korkuluk, bahce.
	AddDecor(DefaultCube, FVector(0, 0, Top - 2.5f), FVector(2.f * HX + 6.f, 2.f * HY + 6.f, 5.f), TrimColor);
	AddDecor(DefaultCube, FVector(0, 0, Top + 0.5f), FVector(2.f * HX, 2.f * HY, 1.f), FLinearColor(0.86f, 0.80f, 0.74f));
	const float Rail = 6.f;
	for (int32 S = -1; S <= 1; S += 2)
	{
		AddDecor(DefaultCube, FVector(S * (HX + 1.5f), 0, Top + 0.5f * Rail), FVector(3.f, 2.f * HY + 6.f, Rail), Tint * 0.95f);
		AddDecor(DefaultCube, FVector(0, S * (HY + 1.5f), Top + 0.5f * Rail), FVector(2.f * HX + 6.f, 3.f, Rail), Tint * 0.95f);
	}
	const float RoofBase = Top + 1.f;
	AddRoundTree(FVector(-0.55f * HX, -0.55f * HY, RoofBase), 30.f);
	AddRoundTree(FVector(-0.55f * HX, 0.5f * HY, RoofBase), 26.f);
	AddCypress(FVector(-0.2f * HX, 0.8f * HY, RoofBase), 28.f);
	AddPlanter(FVector(0.35f * HX, -0.6f * HY, RoofBase), FVector(8.f, 24.f, 5.f), 5);
	AddPlanter(FVector(0.35f * HX, 0.6f * HY, RoofBase), FVector(8.f, 24.f, 5.f), 5);
	AddDecor(DefaultCube, FVector(0.1f * HX, 0, RoofBase + 4.f), FVector(6.f, 22.f, 1.6f), FLinearColor(0.80f, 0.55f, 0.40f)); // bank
	AddDecor(DefaultCube, FVector(0.1f * HX - 3.f, 0, RoofBase + 7.f), FVector(1.4f, 22.f, 5.f), FLinearColor(0.80f, 0.55f, 0.40f));
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

	if (!DisplayMesh && TypeId != TEXT("Durak")) // durakta kapi yok: sundurma onu acik
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
			// Komsu parcalar ust uste biniyor: tek/cift parca RoadZStepCm farkli yukseklikte (z-fighting).
			const float SegH = FlatHeightCm - (I % 2) * RoadZStepCm;
			Seg->SetRelativeScale3D(FVector(Chord, Core, SegH) / 100.f);
			Seg->SetRelativeLocation(FVector(P.X, P.Y, 0.5f * SegH));
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
		const float DiscH = FlatHeightCm - RoadZStepCm; // yarim karonun altinda: ust yuzleri cakismasin
		Disc->SetRelativeScale3D(FVector(Core, Core, DiscH) / 100.f);
		Disc->SetRelativeLocation(FVector(0.f, 0.f, 0.5f * DiscH));
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
	// Uzantilar: kendi karosuyla 0.5 cm, komsunun uzantisiyla 1 cm ust uste biner. Ayni yukseklikte
	// ust yuzler z-fighting yapiyordu (parlama dalgasi gecerken titreme). Uzanti karodan alcak, komsu
	// iki yolun uzantisi da hucre paritesine gore birbirinden alcak/yuksek.
	const float LinkH = FlatHeightCm - RoadZStepCm * (1 + ((CellMin.X + CellMin.Y) & 1));
	const float Core = PlateSizeX();
	const float Len = 0.5f * TileGap + 1.f;
	for (int32 I = 0; I < RoadLinks.Num(); ++I)
	{
		if (RoadLinks[I])
		{
			const bool bAlongX = (I % 2) == 0;
			const FVector Loc = RoadLinks[I]->GetRelativeLocation();
			RoadLinks[I]->SetRelativeScale3D(FVector(bAlongX ? Len : Core, bAlongX ? Core : Len, LinkH) / 100.f);
			RoadLinks[I]->SetRelativeLocation(FVector(Loc.X, Loc.Y, 0.5f * LinkH));
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
	ClearDecor();     // hayalet yalniz govde + plaka
	Mesh->SetVisibility(true); // durak govdesini gizler; hayalette kutu gorunsun
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
	// Figursuz ev (aile aktoru yuruyor, 16 Eki): bos liste "hepsi vardi" sayiliyordu, ev aile gelmeden doluyordu.
	if (!bResidentsStarted || bResidentsArrived || ResidentMeshes.Num() == 0)
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

FVector ASLPlaceable::GetDoorSpotLocal() const
{
	// Kapi yuzunun (+X) plakadaki onu, giris yolunun yaninda (yolu kapatmasin).
	const float PX = 0.5f * PlateSizeX(), PY = 0.5f * PlateSizeY();
	const FVector LocalSpot(PX - 3.f, -FMath::Min(PY - 6.f, BodyHalfY() * 0.75f + 6.f), 0.f);
	return GetRootComponent()->GetRelativeTransform().TransformPosition(LocalSpot) * FVector(1, 1, 0);
}

TArray<FVector> ASLPlaceable::GetArrivalPath(const FVector& StartPlatformLocal) const
{
	// StartResidentsArrival ile ayni kural, platform yerelinde: kapi aileye donuk degilse once yan sokak.
	const FTransform Rel = GetRootComponent()->GetRelativeTransform();
	const FVector Start = Rel.InverseTransformPosition(StartPlatformLocal);
	const float Gap = 12.f;
	const float PlateHalfX = 0.5f * PlateSizeX(), PlateHalfY = 0.5f * PlateSizeY();
	TArray<FVector> Local;
	if (Start.X < PlateHalfX + Gap)
	{
		const float S = Start.Y >= 0.f ? 1.f : -1.f;
		const float Lane = S * FMath::Max(FMath::Abs(Start.Y), PlateHalfY + Gap);
		Local.Add(FVector(Start.X, Lane, 0.f));
		Local.Add(FVector(PlateHalfX + Gap, Lane, 0.f));
	}
	TArray<FVector> Out;
	for (const FVector& L : Local)
	{
		Out.Add(Rel.TransformPosition(L) * FVector(1, 1, 0));
	}
	Out.Add(GetDoorSpotLocal());
	return Out;
}

void ASLPlaceable::PlayPop()
{
	if (bGhost)
	{
		return;
	}
	PopTime = 0.f;
	SetActorTickEnabled(true);
}

bool ASLPlaceable::TickPop(float DeltaSeconds)
{
	if (PopTime < 0.f)
	{
		return false;
	}
	PopTime += DeltaSeconds;
	const float T = FMath::Clamp(PopTime / 0.32f, 0.f, 1.f);
	// Asagi basik basla, hafif tas, otur: sonumlu yay.
	const float S = 1.f - 0.12f * FMath::Exp(-6.f * T) * FMath::Cos(T * 9.f);
	SetActorRelativeScale3D(FVector(1.f / FMath::Sqrt(S), 1.f / FMath::Sqrt(S), S));
	if (T >= 1.f)
	{
		SetActorRelativeScale3D(FVector::OneVector);
		PopTime = -1.f;
		return false;
	}
	return true;
}

void ASLPlaceable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bWalking = TickResidents(DeltaSeconds);
	const bool bFx = TickFx(DeltaSeconds);
	const bool bPop = TickPop(DeltaSeconds);
	if (!bWalking && !bFx && !bPop)
	{
		SetActorTickEnabled(false);
	}
}
