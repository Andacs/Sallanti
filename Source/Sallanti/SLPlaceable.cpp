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

void ASLPlaceable::AddWindowGrid(int32 Face, float Bottom, float Top, int32 Rows, int32 Cols, float WinW, float WinH, bool bSkipDoorColumn)
{
	// Yuzler: 0 +X (kapi), 1 +Y, 2 -X, 3 -Y. Pencere govdeden 0.75 cm disari tasan ince kutu.
	const bool bAlongX = (Face % 2) == 1; // +-Y yuzlerinde pencereler X boyunca dizilir
	const float Sign = (Face < 2) ? 1.f : -1.f;
	const float Off = (bAlongX ? BodyHalfY() : BodyHalfX()) + 0.4f;
	const float Half = bAlongX ? BodyHalfX() : BodyHalfY();
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
			const FVector Loc = bAlongX ? FVector(T, Sign * Off, Z) : FVector(Sign * Off, T, Z);
			const FVector Size = bAlongX ? FVector(WinW, 1.5f, WinH) : FVector(1.5f, WinW, WinH);
			AddWindowPart(Loc, Size);
		}
	}
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
	const float HX = BodyHalfX(), HY = BodyHalfY();
	const float Base = PlateHeightCm;
	const float H = SizeCells.Z * GridSize;
	const float Top = Base + H;
	const float PX = 0.5f * PlateSizeX(), PY = 0.5f * PlateSizeY();

	// Plakanin arka koselerinde servi, on koselerde cali (kapinin onu bos kalir).
	const float Margin = 0.5f * BodyInsetCm;
	const float TreeH = FMath::Clamp(H * 0.55f, 22.f, 60.f);
	AddCypress(FVector(-PX + Margin, -PY + Margin, Base), TreeH);
	AddCypress(FVector(-PX + Margin, PY - Margin, Base), TreeH * 0.85f);
	const float BushD = FMath::Max(5.f, BodyInsetCm * 0.8f);
	AddDecor(SphereMesh, FVector(PX - Margin, PY - Margin, Base + 0.3f * BushD), FVector(BushD, BushD, BushD * 0.75f), BushColor);
	AddDecor(SphereMesh, FVector(PX - Margin, -PY + Margin, Base + 0.3f * BushD), FVector(BushD, BushD, BushD * 0.75f), BushColor);

	if (TypeId == TEXT("Ev"))
	{
		// Bes kenarli cati: govde boyunca 45 derece cevrilmis kup (ust yarisi ucgen alinlik), ustune iki
		// mercan cati levhasi, bir baca. Mahya kapiya dik (X boyunca): alinlik kapinin ustunde.
		const float W = 2.f * HY;
		const float S = W / UE_SQRT_2;
		AddDecor(DefaultCube, FVector(0, 0, Top), FVector(2.f * HX, S, S), Tint, FRotator(0, 0, 45.f));
		const float SlabLen = 0.5f * W * UE_SQRT_2 + 4.f;
		const float SlabT = 3.f;
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const FVector Mid(0, Side * 0.25f * W, Top + 0.25f * W);
			const FVector N = FVector(0, Side, 1).GetSafeNormal();
			AddDecor(DefaultCube, Mid + N * (0.5f * SlabT) + FVector(0, Side * 1.f, -1.f), FVector(2.f * HX + 6.f, SlabLen, SlabT), RoofColor, FRotator(0, 0, Side * 45.f));
		}
		AddDecor(DefaultCube, FVector(-0.35f * HX, 0.3f * W, Top + 0.4f * W), FVector(6.f, 6.f, 0.5f * W), RoofColor * 0.85f);
		// Pencereler: yanlarda ve arkada birer.
		AddWindowGrid(1, Base + 0.55f * H, Base + 0.55f * H, 1, 1, 0.7f * HX, 0.25f * H, false);
		AddWindowGrid(3, Base + 0.55f * H, Base + 0.55f * H, 1, 1, 0.7f * HX, 0.25f * H, false);
		AddWindowGrid(2, Base + 0.55f * H, Base + 0.55f * H, 1, 1, 0.7f * HY, 0.25f * H, false);
	}
	else if (TypeId == TEXT("Hastane"))
	{
		// Duz cati kenari, on yuzde kapinin ustunde kirmizi hac, catida da bir hac. Pencere siralari.
		AddDecor(DefaultCube, FVector(0, 0, Top + 2.f), FVector(2.f * HX + 4.f, 2.f * HY + 4.f, 4.f), Tint * 0.97f);
		const float CrossL = 0.45f * HY * 2.f, CrossT = CrossL * 0.32f;
		const float CrossZ = Base + 0.7f * H;
		AddDecor(DefaultCube, FVector(HX + 1.f, 0, CrossZ), FVector(2.f, CrossL, CrossT), RoofColor);
		AddDecor(DefaultCube, FVector(HX + 1.f, 0, CrossZ), FVector(2.f, CrossT, CrossL), RoofColor);
		AddDecor(DefaultCube, FVector(0, 0, Top + 4.5f), FVector(CrossL, CrossT, 1.5f), RoofColor);
		AddDecor(DefaultCube, FVector(0, 0, Top + 4.5f), FVector(CrossT, CrossL, 1.5f), RoofColor);
		const float WinW = 0.18f * HY * 2.f, WinH = 0.09f * H;
		for (int32 F = 1; F < 4; ++F)
		{
			AddWindowGrid(F, Base + 0.22f * H, Base + 0.85f * H, 4, 3, WinW, WinH, false);
		}
	}
	else if (TypeId == TEXT("AVM"))
	{
		// Zemin katta cam vitrin bandi, kapinin ustunde turkuaz tente ve "AVM" tabelasi, ust katlarda pencere.
		const float GlassH = 0.28f * H;
		for (int32 F = 1; F < 4; ++F) // 0 = kapi yuzu: vitrin yok
		{
			const bool bAlongX = (F % 2) == 1;
			const float Sign = (F < 2) ? 1.f : -1.f;
			const float Off = (bAlongX ? HY : HX) + 0.4f;
			const float Len = 2.f * (bAlongX ? HX : HY) * 0.86f;
			AddWindowPart(bAlongX ? FVector(0, Sign * Off, Base + 0.5f * GlassH + 4.f) : FVector(Sign * Off, 0, Base + 0.5f * GlassH + 4.f),
				bAlongX ? FVector(Len, 1.5f, GlassH) : FVector(1.5f, Len, GlassH));
			AddWindowGrid(F, Base + 0.52f * H, Base + 0.85f * H, 3, 4, 0.13f * Len, 0.08f * H, false);
		}
		const float DoorH = FMath::Min(H * 0.55f, 90.f);
		AddDecor(DefaultCube, FVector(HX + 6.f, 0, Base + DoorH + 3.f), FVector(12.f, 0.7f * HY * 2.f, 3.f), AccentColor, FRotator(-15.f, 0, 0));
		const float SignZ = Base + 0.8f * H;
		AddDecor(DefaultCube, FVector(HX + 1.5f, 0, SignZ), FVector(3.f, 0.6f * HY * 2.f, 0.16f * H), RoofColor);
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
		Text->SetupAttachment(GetRootComponent());
		Text->RegisterComponent();
		Text->SetText(FText::FromString(TEXT("AVM")));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(0.12f * H);
		Text->SetTextRenderColor(FColor(255, 248, 235));
		Text->SetRelativeLocation(FVector(HX + 3.2f, 0, SignZ));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->SetCastShadow(false);
		DecorParts.Add(Text);
		AddDecor(DefaultCube, FVector(0, 0, Top + 2.f), FVector(2.f * HX + 4.f, 2.f * HY + 4.f, 4.f), Tint * 0.95f);
	}
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
