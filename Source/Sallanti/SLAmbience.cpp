#include "SLAmbience.h"

#include "SLGameMode.h"
#include "SLLevelDirector.h"
#include "SLPlaceable.h"
#include "SLPlatform.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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

namespace
{
	constexpr float WrapHalf = 3200.f; // bulut/balon bu kadar uzaklasinca ruzgarin geldigi yana sarar
}

ASLAmbience::ASLAmbience()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Cardboard(TEXT("/Game/Sallanti/Materials/M_Cardboard.M_Cardboard"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ghost(TEXT("/Game/Sallanti/Materials/M_Ghost.M_Ghost"));
	Cube = CubeMesh.Object;
	Sphere = SphereMesh.Object;
	Cone = ConeMesh.Object;
	Cylinder = CylMesh.Object;
	LitMat = Cardboard.Object;
	UnlitMat = Ghost.Object; // isiksiz, Color + Opacity: arka plan, bulut, yildiz
	SetActorEnableCollision(false);
}

USceneComponent* ASLAmbience::AddGroup(const FVector& Loc)
{
	USceneComponent* G = NewObject<USceneComponent>(this);
	G->SetMobility(EComponentMobility::Movable);
	G->SetupAttachment(GetRootComponent());
	G->RegisterComponent();
	G->SetRelativeLocation(Loc);
	return G;
}

UStaticMeshComponent* ASLAmbience::AddShape(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FLinearColor& Color, bool bUnlit, float Opacity, const FRotator& Rot)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(Mesh);
	C->SetMobility(EComponentMobility::Movable);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false); // arka plan tahtaya golge dusurmesin
	C->SetupAttachment(Parent ? Parent : GetRootComponent());
	C->RegisterComponent();
	C->SetRelativeRotation(Rot);
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

void ASLAmbience::AddCypress(USceneComponent* Parent, const FVector& Base, float Height, const FLinearColor& Color)
{
	const float W = Height * 0.3f;
	const float Trunk = Height * 0.12f;
	AddShape(Parent, Cube, Base + FVector(0, 0, 0.5f * Trunk), FVector(W * 0.22f, W * 0.22f, Trunk), FLinearColor(0.50f, 0.36f, 0.26f), false);
	const float Crown = Height - Trunk;
	AddShape(Parent, Cone, Base + FVector(0, 0, Trunk + 0.55f * Crown), FVector(W, W, Crown * 0.9f), Color, false);
	AddShape(Parent, Sphere, Base + FVector(0, 0, Trunk + 0.2f * Crown), FVector(W, W, Crown * 0.4f), Color, false);
}

void ASLAmbience::BuildGardenIsland(const FVector& Center, float Size, FRandomStream& Rng)
{
	// Binasiz bahce adacigi (16 Eki, Andac: "kartonun disindaki binalari kaldir"): toprak kaide, cimen,
	// agaclar, cicekler. Kendi temposunda yukari asagi sallanir.
	USceneComponent* G = AddGroup(Center);
	const float BaseH = Size * 0.3f;
	AddShape(G, Cylinder, FVector(0, 0, -0.5f * BaseH), FVector(Size, Size, BaseH), IslandColor * 0.95f, false);
	AddShape(G, Cone, FVector(0, 0, -BaseH - 0.3f * Size), FVector(Size * 0.8f, Size * 0.8f, Size * 0.6f), IslandColor * 0.85f, false, 1.f, FRotator(180.f, 0, 0));
	AddShape(G, Cylinder, FVector(0, 0, 1.f), FVector(Size * 0.96f, Size * 0.96f, 3.f), GrassColor, false);
	const int32 Trees = Rng.RandRange(2, 4);
	for (int32 I = 0; I < Trees; ++I)
	{
		const float A = Rng.FRandRange(0.f, UE_TWO_PI), R = Rng.FRandRange(0.05f, 0.33f) * Size;
		const FVector P(FMath::Cos(A) * R, FMath::Sin(A) * R, 2.f);
		if (Rng.FRand() < 0.5f)
		{
			AddCypress(G, P, Size * Rng.FRandRange(0.35f, 0.6f), TreeColor);
		}
		else
		{
			const float H = Size * Rng.FRandRange(0.3f, 0.45f);
			AddShape(G, Cube, P + FVector(0, 0, 0.3f * H), FVector(0.06f * H, 0.06f * H, 0.6f * H), FLinearColor(0.50f, 0.36f, 0.26f), false);
			AddShape(G, Sphere, P + FVector(0, 0, 0.75f * H), FVector(0.6f * H, 0.6f * H, 0.55f * H), GrassColor * 0.85f, false);
		}
	}
	const FLinearColor Flowers[3] = { FLinearColor(0.98f, 0.55f, 0.60f), FLinearColor(1.f, 0.85f, 0.35f), FLinearColor(0.80f, 0.60f, 0.95f) };
	for (int32 I = 0; I < 7; ++I)
	{
		const float A = Rng.FRandRange(0.f, UE_TWO_PI), R = Rng.FRandRange(0.1f, 0.42f) * Size;
		AddShape(G, Sphere, FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 4.f), FVector(6.f, 6.f, 5.f), Flowers[I % 3], false);
	}
	FDrifter D;
	D.Comp = G;
	D.Base = Center;
	D.BobAmp = Rng.FRandRange(8.f, 18.f);
	D.BobHz = Rng.FRandRange(0.08f, 0.15f);
	D.Phase = Rng.FRandRange(0.f, UE_TWO_PI);
	Drifters.Add(D);
}

void ASLAmbience::BuildBalloon(const FVector& Center, float Size, const FLinearColor& Color, FRandomStream& Rng)
{
	// Sicak hava balonu: zarf (basik kure), beyaz serit, alt koni, sepet, dort ip. Ruzgarla suzulur.
	USceneComponent* G = AddGroup(Center);
	AddShape(G, Sphere, FVector(0, 0, 0.6f * Size), FVector(Size, Size, 1.1f * Size), Color, false);
	AddShape(G, Cylinder, FVector(0, 0, 0.6f * Size), FVector(1.02f * Size, 1.02f * Size, 0.14f * Size), FLinearColor(0.99f, 0.96f, 0.92f), false);
	AddShape(G, Cone, FVector(0, 0, 0.02f * Size), FVector(0.55f * Size, 0.55f * Size, 0.3f * Size), Color * 0.9f, false, 1.f, FRotator(180.f, 0, 0));
	AddShape(G, Cube, FVector(0, 0, -0.42f * Size), FVector(0.22f * Size, 0.22f * Size, 0.16f * Size), FLinearColor(0.72f, 0.50f, 0.34f), false);
	for (int32 I = 0; I < 4; ++I)
	{
		const float X = (I < 2 ? 1.f : -1.f) * 0.1f * Size, Y = (I % 2 ? 1.f : -1.f) * 0.1f * Size;
		AddShape(G, Cube, FVector(X, Y, -0.22f * Size), FVector(0.8f, 0.8f, 0.3f * Size), FLinearColor(0.55f, 0.45f, 0.40f), false);
	}
	FDrifter D;
	D.Comp = G;
	D.Base = Center;
	D.Speed = Rng.FRandRange(14.f, 26.f);
	D.BobAmp = Rng.FRandRange(15.f, 30.f);
	D.BobHz = Rng.FRandRange(0.05f, 0.1f);
	D.Phase = Rng.FRandRange(0.f, UE_TWO_PI);
	Drifters.Add(D);
}

void ASLAmbience::BuildFlock(int32 Index, FRandomStream& Rng)
{
	// Kus surusu: V dizilisinde 5 kus, tahtanin cevresinde genis cember, kanat cirpar.
	FlockParams.Add(FVector(Rng.FRandRange(900.f, 1300.f), Rng.FRandRange(0.06f, 0.1f) * (Index % 2 ? -1.f : 1.f), Rng.FRandRange(-260.f, -120.f)));
	const FLinearColor C(0.30f, 0.28f, 0.36f);
	for (int32 I = 0; I < 5; ++I)
	{
		USceneComponent* B = AddGroup(FVector::ZeroVector);
		AddShape(B, Sphere, FVector::ZeroVector, FVector(10.f, 4.f, 4.f), C, false);
		auto Wing = [&](float Side)
		{
			USceneComponent* W = NewObject<USceneComponent>(this);
			W->SetupAttachment(B);
			W->RegisterComponent();
			AddShape(W, Cube, FVector(0, Side * 7.f, 0), FVector(5.f, 14.f, 0.8f), C, false);
			return W;
		};
		FBird Bd;
		Bd.Comp = B;
		Bd.WingL = Wing(-1.f);
		Bd.WingR = Wing(1.f);
		const int32 Row = (I + 1) / 2;
		Bd.Offset = FVector(-Row * 26.f, (I % 2 ? 1.f : -1.f) * Row * 22.f, Rng.FRandRange(-5.f, 5.f));
		Bd.Flock = Index;
		Bd.FlapPhase = Rng.FRandRange(0.f, UE_TWO_PI);
		Birds.Add(Bd);
	}
}

float ASLAmbience::DaylightAt(float Hour)
{
	const float H = FMath::Fmod(FMath::Fmod(Hour, 24.f) + 24.f, 24.f);
	if (H >= 8.f && H <= 17.f)
	{
		return 1.f;
	}
	if (H > 5.f && H < 8.f)
	{
		return FMath::SmoothStep(5.f, 8.f, H);
	}
	if (H > 17.f && H < 20.f)
	{
		return 1.f - FMath::SmoothStep(17.f, 20.f, H);
	}
	return 0.f;
}

float ASLAmbience::TargetHours() const
{
	if (DebugHourOverride >= 0.f)
	{
		return DebugHourOverride;
	}
	const ASLGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASLGameMode>() : nullptr;
	const ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
	return D ? D->GetTimeHours() : 12.f;
}

void ASLAmbience::ApplyTimeOfDay(bool bForce)
{
	const float H = FMath::Fmod(ShownHours, 24.f);
	Daylight = DaylightAt(H);
	const float Night = 1.f - Daylight;
	// Pencereler her karede: yeni konan bina da gecenin rengini hemen alsin.
	const ASLGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASLGameMode>() : nullptr;
	if (const ASLPlatform* P = GM ? GM->GetPlatform() : nullptr)
	{
		const float Glow = FMath::Clamp(Night * 1.3f, 0.f, 1.f);
		for (const TWeakObjectPtr<ASLPlaceable>& W : P->GetPlaceables())
		{
			if (ASLPlaceable* Pl = W.Get())
			{
				Pl->SetNightGlow(Glow);
			}
		}
	}
	if (!bForce && FMath::Abs(ShownHours - LastAppliedHours) < 0.01f)
	{
		return;
	}
	LastAppliedHours = ShownHours;
	const float Dusk = FMath::Clamp(1.f - FMath::Abs(Daylight - 0.45f) / 0.45f, 0.f, 1.f) * (Daylight < 1.f ? 1.f : 0.f);

	FLinearColor Sky = FMath::Lerp(BackdropNightColor, BackdropColor, Daylight);
	Sky = FMath::Lerp(Sky, DuskColor, 0.45f * Dusk);
	if (BackdropMID)
	{
		BackdropMID->SetVectorParameterValue(TEXT("Color"), Sky * UnlitBrightness);
	}
	const FLinearColor Cloud = FMath::Lerp(FMath::Lerp(CloudNightColor, CloudColor, Daylight), DuskColor, 0.3f * Dusk);
	for (const FCloudPuff& P : Puffs)
	{
		if (UMaterialInstanceDynamic* M = P.MID.Get())
		{
			M->SetVectorParameterValue(TEXT("Color"), Cloud * UnlitBrightness);
		}
	}
	if (StarMID)
	{
		StarMID->SetScalarParameterValue(TEXT("Opacity"), 0.9f * FMath::Clamp(Night * 1.4f - 0.3f, 0.f, 1.f));
	}

	if (bTintLevelLights && GetWorld())
	{
		// Gunduz: gunes sabah dogudan alcak, oglen SunRotation, aksam batidan alcak. Gece: sabit ay isigi.
		// 14 derecede aksam 18:00'de tahta kararip arka plan acik kaliyordu (ekran goruntusu): en alcak 32.
		const float DayT = FMath::Clamp((H - 5.f) / 15.f, 0.f, 1.f);
		const float Elev = FMath::Lerp(32.f, -SunRotation.Pitch, FMath::Sin(DayT * UE_PI));
		const float Yaw = SunRotation.Yaw + FMath::Lerp(-45.f, 45.f, DayT);
		const FRotator SunRot(-Elev, Yaw, 0.f);
		const FRotator MoonRot(-55.f, SunRotation.Yaw + 10.f, 0.f);
		const bool bSun = Daylight > 0.02f;
		const FLinearColor SunC = FMath::Lerp(SunColor, SunsetColor, Dusk);
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
			{
				L->SetLightColor(bSun ? FMath::Lerp(MoonColor, SunC, FMath::Sqrt(Daylight)) : MoonColor);
				L->SetIntensity(FMath::Lerp(MoonIntensity, SunIntensity, Daylight));
				It->SetActorRotation(bSun ? SunRot : MoonRot);
			}
		}
		for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
		{
			if (USkyLightComponent* L = It->GetLightComponent())
			{
				L->SetLightColor(FMath::Lerp(SkyLightNightColor, SkyLightColor, Daylight));
				L->SetIntensity(FMath::Lerp(SkyLightNightIntensity, SkyLightIntensity, Daylight));
			}
		}
		for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
		{
			if (UExponentialHeightFogComponent* F = It->GetComponent())
			{
				F->SetFogInscatteringColor(Sky);
			}
		}
	}
}

void ASLAmbience::BeginPlay()
{
	Super::BeginPlay();
	FRandomStream Rng(RandomSeed);

	// Arka plan: tahtanin cok altinda dev, isiksiz bir duzlem. -45 derece bakista ekranin geri kalani bu.
	UStaticMeshComponent* Back = AddShape(nullptr, Cube, FVector(0, 0, -2600.f), FVector(30000.f, 30000.f, 10.f), BackdropColor, true, 1.f);
	BackdropMID = Cast<UMaterialInstanceDynamic>(Back->GetMaterial(0));

	// Yildizlar: arka planin hemen ustunde kucuk isiksiz noktalar; yalnizca gece gorunur.
	Stars = NewObject<UInstancedStaticMeshComponent>(this);
	Stars->SetStaticMesh(Sphere);
	Stars->SetMobility(EComponentMobility::Movable);
	Stars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stars->SetCastShadow(false);
	Stars->SetupAttachment(GetRootComponent());
	Stars->RegisterComponent();
	for (int32 I = 0; I < StarCount; ++I)
	{
		const FVector P(Rng.FRandRange(-2500.f, 7000.f), Rng.FRandRange(-2500.f, 7000.f), -2580.f);
		const float S = Rng.FRandRange(14.f, 34.f) / 100.f;
		Stars->AddInstance(FTransform(FQuat::Identity, P, FVector(S, S, 0.05f)));
	}
	StarMID = UMaterialInstanceDynamic::Create(UnlitMat, Stars);
	if (StarMID)
	{
		StarMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.96f, 0.85f) * 3.f);
		StarMID->SetScalarParameterValue(TEXT("Opacity"), 0.f);
		Stars->SetMaterial(0, StarMID);
	}

	// Bulutlar: uc katman (uzak, orta, yakin). Uzaktaki daha yavas: kamera kipirdamasa da derinlik hissi.
	for (int32 I = 0; I < CloudCount; ++I)
	{
		const int32 Layer = I % 3;
		const float Z = Layer == 0 ? Rng.FRandRange(-1500.f, -1000.f) : Layer == 1 ? Rng.FRandRange(-950.f, -600.f) : Rng.FRandRange(-500.f, -320.f);
		const FVector C(Rng.FRandRange(-2600.f, 2600.f), Rng.FRandRange(-2600.f, 2600.f), Z);
		if (Layer == 2 && FVector2D(C).Size() < 700.f)
		{
			continue; // yakin katman tahtanin hemen altini kapatmasin
		}
		USceneComponent* G = AddGroup(C);
		const int32 PuffN = Rng.RandRange(3, 5);
		const float S = Rng.FRandRange(160.f, 360.f) * (Layer == 2 ? 0.7f : 1.f);
		for (int32 P = 0; P < PuffN; ++P)
		{
			const FVector O(Rng.FRandRange(-0.9f, 0.9f) * S, Rng.FRandRange(-0.5f, 0.5f) * S, Rng.FRandRange(-0.1f, 0.15f) * S);
			const float PS = S * Rng.FRandRange(0.6f, 1.1f);
			const float Op = Rng.FRandRange(0.6f, 0.9f) * (Layer == 0 ? 0.8f : 1.f);
			UStaticMeshComponent* Puff = AddShape(G, Sphere, O, FVector(PS * 1.4f, PS, PS * 0.45f), CloudColor, true, Op);
			Puffs.Add({ Cast<UMaterialInstanceDynamic>(Puff->GetMaterial(0)), Op });
		}
		FDrifter D;
		D.Comp = G;
		D.Base = C;
		D.Speed = LayerSpeeds[Layer] * Rng.FRandRange(0.8f, 1.2f);
		D.BobAmp = 6.f;
		D.BobHz = 0.05f;
		D.Phase = Rng.FRandRange(0.f, UE_TWO_PI);
		Drifters.Add(D);
	}

	// Bahce adaciklari: kameranin iki yaninda (L = yana (0.707,-0.707), D = derinlik (0.707,0.707)).
	const FVector LDZ[] = {
		{ 760.f, -150.f, -320.f }, { 820.f, 520.f, -460.f }, { -760.f, -100.f, -300.f },
		{ -840.f, 560.f, -480.f }, { 150.f, 1700.f, -700.f }, { -400.f, -950.f, -650.f },
	};
	const FVector2D Side(0.7071f, -0.7071f), Depth(0.7071f, 0.7071f);
	for (const FVector& V : LDZ)
	{
		const FVector2D P = Side * V.X + Depth * V.Y;
		BuildGardenIsland(FVector(P.X, P.Y, V.Z), Rng.FRandRange(110.f, 160.f), Rng);
	}
	// Balonlar: farkli derinlik ve yukseklikte; ruzgarla suzulur, kadrajdan cikinca diger yandan girer.
	for (int32 I = 0; I < BalloonCount; ++I)
	{
		const FVector2D P = Side * Rng.FRandRange(-1000.f, 1000.f) + Depth * Rng.FRandRange(-300.f, 1800.f);
		const FLinearColor Col = BalloonColors.Num() ? BalloonColors[I % BalloonColors.Num()] : FLinearColor::White;
		BuildBalloon(FVector(P.X, P.Y, Rng.FRandRange(-350.f, 150.f)), Rng.FRandRange(45.f, 80.f), Col, Rng);
	}
	for (int32 I = 0; I < FlockCount; ++I)
	{
		BuildFlock(I, Rng);
	}
	ShownHours = TargetHours();
	bTimeInit = true;
	ApplyTimeOfDay(true);
}

void ASLAmbience::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	const FVector2D Wind = WindDir.GetSafeNormal();
	for (FDrifter& D : Drifters)
	{
		USceneComponent* C = D.Comp.Get();
		if (!C)
		{
			continue;
		}
		D.Travel += D.Speed * DeltaSeconds;
		// Ruzgar ekseninde -WrapHalf..+WrapHalf arasinda sarar.
		const float Along = FVector2D::DotProduct(FVector2D(D.Base), Wind) + D.Travel;
		if (Along > WrapHalf)
		{
			D.Travel -= 2.f * WrapHalf;
		}
		const FVector2D XY = FVector2D(D.Base) + Wind * D.Travel;
		const float Bob = D.BobAmp * FMath::Sin(UE_TWO_PI * D.BobHz * Time + D.Phase);
		C->SetRelativeLocation(FVector(XY.X, XY.Y, D.Base.Z + Bob));
	}
	for (FBird& B : Birds)
	{
		USceneComponent* C = B.Comp.Get();
		if (!C || !FlockParams.IsValidIndex(B.Flock))
		{
			continue;
		}
		const FVector& F = FlockParams[B.Flock];
		const float A = F.Y * Time + B.Flock * 2.1f;
		const float Dir = F.Y >= 0.f ? 1.f : -1.f;
		const FVector Center(FMath::Cos(A) * F.X, FMath::Sin(A) * F.X, F.Z + 20.f * FMath::Sin(Time * 0.3f + B.Flock));
		const float Heading = FMath::RadiansToDegrees(A) + 90.f * Dir;
		const FRotator Rot(0.f, Heading, 0.f);
		C->SetRelativeLocation(Center + Rot.RotateVector(B.Offset));
		C->SetRelativeRotation(Rot);
		const float Flap = 30.f * FMath::Sin(Time * 7.f + B.FlapPhase);
		if (USceneComponent* L = B.WingL.Get()) { L->SetRelativeRotation(FRotator(0, 0, -Flap)); }
		if (USceneComponent* R = B.WingR.Get()) { R->SetRelativeRotation(FRotator(0, 0, Flap)); }
	}
	// Saat: hamleden sonra hedef saate dogru akar (gunes kayar, gokyuzu renk degistirir).
	const float Target = TargetHours();
	if (DebugHourOverride >= 0.f)
	{
		ShownHours = Target;
	}
	else if (ShownHours < Target)
	{
		ShownHours = FMath::Min(Target, ShownHours + TimeLerpHoursPerSecond * DeltaSeconds);
	}
	else
	{
		ShownHours = Target; // bolum yeniden basladi
	}
	ApplyTimeOfDay(false);
}
