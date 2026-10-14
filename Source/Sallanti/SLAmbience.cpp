#include "SLAmbience.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASLAmbience::ASLAmbience()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Cardboard(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ghost(TEXT("/Game/Sallanti/Materials/M_Ghost.M_Ghost"));
	Cube = CubeMesh.Object;
	Sphere = SphereMesh.Object;
	Cone = ConeMesh.Object;
	LitMat = Cardboard.Object;
	UnlitMat = Ghost.Object; // isiksiz, Color + Opacity: arka plan ve bulut icin
	SetActorEnableCollision(false);
}

UStaticMeshComponent* ASLAmbience::AddShape(UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color, bool bUnlit, float Opacity)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(Mesh);
	C->SetMobility(EComponentMobility::Movable);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(!bUnlit);
	C->SetupAttachment(GetRootComponent());
	C->RegisterComponent();
	C->SetRelativeLocation(Loc);
	C->SetRelativeScale3D(SizeCm / 100.f); // motorun hazir sekilleri 100 cm
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(bUnlit ? UnlitMat.Get() : LitMat.Get(), C))
	{
		if (bUnlit)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color * UnlitBrightness);
			MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		}
		else
		{
			MID->SetVectorParameterValue(TEXT("Tint"), Color);
		}
		C->SetMaterial(0, MID);
	}
	return C;
}

void ASLAmbience::AddCypress(AActor* Owner, USceneComponent* Parent, UStaticMesh* InCone, UStaticMesh* InCube,
	UMaterialInterface* Mat, const FVector& Base, float Height, const FLinearColor& Color)
{
	if (!Owner || !Parent || !InCone || !InCube || !Mat)
	{
		return;
	}
	auto Make = [&](UStaticMesh* M, const FVector& Loc, const FVector& Size, const FLinearColor& Col)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetStaticMesh(M);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(Parent);
		C->RegisterComponent();
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Size / 100.f);
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, C))
		{
			MID->SetVectorParameterValue(TEXT("Tint"), Col);
			C->SetMaterial(0, MID);
		}
	};
	const float W = Height * 0.28f;
	const float Trunk = Height * 0.12f;
	Make(InCube, Base + FVector(0, 0, 0.5f * Trunk), FVector(W * 0.25f, W * 0.25f, Trunk), FLinearColor(0.45f, 0.32f, 0.25f));
	// Motor konisi 100 cm, merkez pivotlu: tabani govdenin ustune oturt.
	const float Crown = Height - Trunk;
	Make(InCone, Base + FVector(0, 0, Trunk + 0.5f * Crown), FVector(W, W, Crown), Color);
}

void ASLAmbience::BuildIsland(const FVector& Center, float Size, FRandomStream& Rng)
{
	// Kaide: kalin krem blok, altinda kucuk bir "tas" (adacik havada asili).
	const float BaseH = Size * 0.35f;
	AddShape(Cube, Center + FVector(0, 0, -0.5f * BaseH), FVector(Size, Size, BaseH), IslandColor * 0.95f, false);
	AddShape(Cube, Center + FVector(0, 0, -BaseH - 0.2f * Size), FVector(Size * 0.55f, Size * 0.55f, Size * 0.4f), IslandColor * 0.85f, false);

	const int32 Houses = Rng.RandRange(1, 3);
	for (int32 I = 0; I < Houses; ++I)
	{
		const float W = Size * Rng.FRandRange(0.28f, 0.42f);
		const float H = W * Rng.FRandRange(1.0f, 2.2f);
		const FVector P = Center + FVector(Rng.FRandRange(-0.25f, 0.25f) * Size, Rng.FRandRange(-0.25f, 0.25f) * Size, 0.f);
		const FLinearColor Body = IslandHouseColors.Num() ? IslandHouseColors[Rng.RandRange(0, IslandHouseColors.Num() - 1)] : IslandColor;
		AddShape(Cube, P + FVector(0, 0, 0.5f * H), FVector(W, W, H), Body, false);
		// Pencereler: on yuzlerde kucuk mavi kareler (iki yone).
		const int32 Rows = FMath::Max(1, FMath::RoundToInt(H / (W * 0.6f)));
		for (int32 R = 0; R < Rows; ++R)
		{
			const float Z = H * (R + 0.6f) / (Rows + 0.4f);
			AddShape(Cube, P + FVector(-0.5f * W - 0.5f, 0.f, Z), FVector(1.f, W * 0.22f, W * 0.3f), WindowColor, false);
			AddShape(Cube, P + FVector(0.f, -0.5f * W - 0.5f, Z), FVector(W * 0.22f, 1.f, W * 0.3f), WindowColor, false);
		}
		// Cati: yarisinda mercan kulah (koni), digerlerinde ince kenar bandi.
		if (Rng.FRand() < 0.5f)
		{
			AddShape(Cone, P + FVector(0, 0, H + 0.35f * W), FVector(W * 1.1f, W * 1.1f, W * 0.7f), IslandRoofColor, false);
		}
		else
		{
			AddShape(Cube, P + FVector(0, 0, H + 2.f), FVector(W * 1.06f, W * 1.06f, 4.f), Body * 0.96f, false);
		}
	}
	const int32 Trees = Rng.RandRange(1, 3);
	for (int32 I = 0; I < Trees; ++I)
	{
		const FVector P = Center + FVector(Rng.FRandRange(-0.4f, 0.4f) * Size, Rng.FRandRange(-0.4f, 0.4f) * Size, 0.f);
		AddCypress(this, GetRootComponent(), Cone, Cube, LitMat, P, Size * Rng.FRandRange(0.35f, 0.6f), TreeColor);
	}
}

void ASLAmbience::TintLights()
{
	if (!bTintLevelLights || !GetWorld())
	{
		return;
	}
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			L->SetLightColor(SunColor);
			L->SetIntensity(SunIntensity);
			It->SetActorRotation(SunRotation);
		}
	}
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* L = It->GetLightComponent())
		{
			L->SetLightColor(SkyLightColor);
			L->SetIntensity(SkyLightIntensity);
		}
	}
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		if (UExponentialHeightFogComponent* F = It->GetComponent())
		{
			F->SetFogInscatteringColor(BackdropColor);
		}
	}
}

void ASLAmbience::BeginPlay()
{
	Super::BeginPlay();
	FRandomStream Rng(RandomSeed);

	// Arka plan: tahtanin cok altinda dev, isiksiz bir duzlem. -45 derece bakista ekranin geri kalani bu.
	AddShape(Cube, FVector(0, 0, -2600.f), FVector(30000.f, 30000.f, 10.f), BackdropColor, true, 1.f);
	// (Yari saydamlar nesne merkezine uzakliga gore siralanir: bulutlar bu duzlemden kameraya yakin, ustte cizilir.)

	// Bulutlar: 3-5 basik kureden kumeler, tahtanin altinda ve cevresinde.
	for (int32 I = 0; I < CloudCount; ++I)
	{
		const float Ang = Rng.FRandRange(0.f, UE_TWO_PI);
		const float Rad = Rng.FRandRange(450.f, 2200.f);
		const FVector C(FMath::Cos(Ang) * Rad, FMath::Sin(Ang) * Rad, Rng.FRandRange(-1400.f, -350.f));
		const int32 Puffs = Rng.RandRange(3, 5);
		const float S = Rng.FRandRange(160.f, 380.f);
		for (int32 P = 0; P < Puffs; ++P)
		{
			const FVector O(Rng.FRandRange(-0.9f, 0.9f) * S, Rng.FRandRange(-0.5f, 0.5f) * S, Rng.FRandRange(-0.1f, 0.15f) * S);
			const float PS = S * Rng.FRandRange(0.6f, 1.1f);
			AddShape(Sphere, C + O, FVector(PS * 1.4f, PS, PS * 0.45f), CloudColor, true, Rng.FRandRange(0.65f, 0.9f));
		}
	}

	// Adaciklar: kameranin sol ve sag kenarinda (ekran sagi = -X+Y, solu = +X-Y), tahtadan asagida.
	// Ekran ekseninde: L = yana (kamera yaw 45: (0.707,-0.707)), D = derinlik ((0.707,0.707), + = tahtanin arkasi).
	// Gorus genisligi 2900 cm'de ~+-780 cm: adaciklar kenarda, yarisi kadrajda kalsin.
	const FVector LDZ[] = {
		{ 760.f, -150.f, -320.f }, { 820.f, 450.f, -420.f }, { 640.f, 1100.f, -560.f },
		{ -760.f, -100.f, -300.f }, { -840.f, 500.f, -440.f }, { -620.f, 1150.f, -600.f },
		{ 150.f, 1700.f, -700.f }, { -350.f, -900.f, -650.f },
	};
	const FVector2D Side(0.7071f, -0.7071f), Depth(0.7071f, 0.7071f);
	for (const FVector& V : LDZ)
	{
		const FVector2D P = Side * V.X + Depth * V.Y;
		BuildIsland(FVector(P.X, P.Y, V.Z), Rng.FRandRange(110.f, 160.f), Rng);
	}
	TintLights();
}
