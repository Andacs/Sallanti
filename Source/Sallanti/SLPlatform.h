// Sallanan platform. Agirlik merkezinin ve egimin TEK sahibi: HUD, ses, kaybetme kontrolu buradan okur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLPlatform.generated.h"

class ASLPlaceable;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESLBalanceState : uint8
{
	Stable,    // WarnTilt altinda
	Warning,   // WarnTilt..FailTilt: platform kirmizi atar
	Failing,   // FailTilt ustunde, FailSeconds sayiyor: hala kurtarilabilir
	Collapsed  // devrildi, oyun bitti
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSLOnCollapsed);

UCLASS()
class SALLANTI_API ASLPlatform : public AActor
{
	GENERATED_BODY()

public:
	ASLPlatform();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool IsCollapsed() const { return BalanceState == ESLBalanceState::Collapsed; }

	/** Yerlestirilen nesneler buna baglanir; egim bu bileseni dondurur. Ust yuzey yerel Z=0. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<USceneComponent> TiltPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UStaticMeshComponent> Deck;

	/** Izgara cizgileri; TiltPivot'a bagli, platformla birlikte egilir. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UInstancedStaticMeshComponent> GridLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Izgara")
	FLinearColor GridColor = FLinearColor(0.12f, 0.08f, 0.05f);

	/** Cizgi kalinligi, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Izgara")
	float GridLineWidth = 1.5f;

	/** true = hep gorunur. false = yalnizca yerlestirirken (BuildComponent acar/kapatir). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Izgara")
	bool bGridAlwaysVisible = false;

	void SetGridShownForPlacing(bool bPlacing);

	/** Boyut/izgara degerleri canli degistirildiyse (SLSet) gorunumu yeniden kurar. */
	void RefreshVisuals() { ApplyDeckSize(); BuildGrid(); }

	// --- Denge (09_MEKANIK_SPEC 7. bolum; hepsi baslangic tahmini) ---

	/** Agirlik merkezi ofseti -> egim, derece/cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge")
	float TiltPerCm = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float MaxTilt = 12.f;

	/** Platformun merkezdeki kendi kutlesi: CoM = Sum(m*r) / (PlatformMass + Sum(m)).
	 *  0 = spec formulu; ilk kutu tek basina tam egim verir. Buyudukce ilk nesneler hafif kalir, yuk birikir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (ClampMin = "0"))
	float PlatformMass = 0.f;

	/** Hedef egime yaklasma hizi, derece/sn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge")
	float TiltSpeed = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float SwayAmplitude = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Seconds", ClampMin = "0.1"))
	float SwayPeriod = 4.f;

	/** Sallanmanin platform uzerindeki yonu (yerel XY'de, derece). 0 = +X kenari iner/kalkar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Denge", meta = (Units = "Degrees"))
	float SwayDirection = 0.f;

	// --- Uyari ve devrilme (14 Ekim tasarim degisikligi: binalar sabit, platform devrilir) ---
	// Egim kararlari sallanma HARIC egime bakar: anlik salinimda uyarmak/kaybettirmek haksiz.

	/** Bu egimden itibaren uyari: platform kirmiziya dogru nabiz gibi atar, FailTilt'e yaklastikca hizlanir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme", meta = (Units = "Degrees"))
	float WarnTilt = 8.f;

	/** Kaybetme esigi. FailSeconds boyunca kesintisiz asilirsa platform devrilir, oyun biter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme", meta = (Units = "Degrees"))
	float FailTilt = 10.f;

	/** Esigin ustunde kalma suresi. Bu sure oyuncunun karsi agirlikla kurtarma penceresi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme", meta = (Units = "Seconds"))
	float FailSeconds = 1.5f;

	/** Devrilmenin vardigi aci. Binalar bagli kalir, platformla birlikte devrilir.
	 *  Olculdu: 75'te kameradan uzaga devrilen platformun alti ekrani kapliyor, binalar kayboluyor.
	 *  Kamera 35 deg yukaridan bakiyor: uzaga devrilmede 35'ten sonra alt yuz gorunur. 45'te alt yuz
	 *  gorunse de binalar kenarin ustunden seciliyor (ekran goruntusu SL_T8_5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme", meta = (Units = "Degrees"))
	float CollapseAngle = 45.f;

	/** Devrilme ivmesi, derece/sn^2. Yavas baslayip hizlanan dusus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme")
	float CollapseAcceleration = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme")
	FLinearColor WarnColor = FLinearColor(0.85f, 0.12f, 0.08f);

	/** Uyari nabzinin gucu: WarnTilt'te Min, FailTilt'e yaklasirken Max'a cikar (0 = karton, 1 = WarnColor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme", meta = (ClampMin = "0", ClampMax = "1"))
	float WarnStrengthMin = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme", meta = (ClampMin = "0", ClampMax = "1"))
	float WarnStrengthMax = 0.9f;

	/** Nabiz hizi, Hz: WarnTilt'te Min, FailTilt'te Max. Devrilme sayaci isliyorken FailingPulseHz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme")
	float WarnPulseHzMin = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme")
	float WarnPulseHzMax = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme")
	float FailingPulseHz = 6.f;

	/** Platformun normal karton rengi (M_Cardboard Tint). Uyari bundan WarnColor'a dogru atar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Devrilme")
	FLinearColor DeckTint = FLinearColor(0.62f, 0.45f, 0.28f);

	// --- Boyut ---

	/** Platform kenari / 2, cm. 300 = 600x600 (08_ASSET_SPEC: 12x12 modul). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Platform")
	float HalfExtent = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Platform")
	float DeckThickness = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Platform")
	float GridSize = 50.f;

	/** Ekrana CoM/egim yazar ve CoM'u isaretler. Ayar yaparken acik kalsin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Debug")
	bool bDebugDraw = true;

	// --- Okunan durum (HUD/ses buradan okur) ---

	/** Yerel XY, cm. Nesne yoksa (0,0). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	FVector2D CenterOfMass = FVector2D::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	float TotalMass = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	ESLBalanceState BalanceState = ESLBalanceState::Stable;

	/** 0 = WarnTilt'te, 1 = FailTilt'te. HUD gostergesi ve ses bunu okur. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	float WarnAlpha = 0.f;

	/** FailTilt'in ustunde kesintisiz gecen sure, sn. FailSeconds'a ulasinca devrilir. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	float FailTimer = 0.f;

	/** Devrildigi an yayinlanir; GameMode oyunu bitirir. */
	UPROPERTY(BlueprintAssignable, Category = "Sallanti")
	FSLOnCollapsed OnCollapsed;

	/** CoM'un istedigi egim (sallanma haric), derece. Mevcut egim buna TiltSpeed ile yaklasir. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	float TargetTiltDegrees = 0.f;

	/** Sallanma haric egim vektoru: yon = inen taraf (yerel XY), boy = derece. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	FVector2D CurrentTilt = FVector2D::ZeroVector;

	/** Sallanma dahil toplam egim, derece (ekranda gorulen). Uyari/kaybetme sallanma haric CurrentTilt'e bakar. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sallanti|Durum")
	float CurrentTiltDegrees = 0.f;

	int32 GetCellCount() const { return FMath::Max(1, FMath::RoundToInt(2.f * HalfExtent / GridSize)); }

	/** Hucre indeksinin (0..N) yerel koordinati. Hucre 0'in sol alt kosesi = -HalfExtent. */
	float CellToLocal(float Cell) const { return -HalfExtent + Cell * GridSize; }

	void RegisterPlaceable(ASLPlaceable* Placeable);
	const TArray<TWeakObjectPtr<ASLPlaceable>>& GetPlaceables() const { return Placeables; }

	/** Ayak izi [Min, Min+Size) hucrelerinin ustundeki en yuksek seviye (hucre cinsinden). */
	int32 GetStackLevel(FIntPoint CellMin, FIntPoint Footprint) const;

	/** Debug/olcum: yerel bir noktanin su anki dunya Z'si. */
	float GetWorldZOfLocal(const FVector& Local) const;

private:
	void RecomputeCenterOfMass();
	void ApplyDeckSize();
	void BuildGrid();
	void UpdateBalanceState(float DeltaSeconds);
	void UpdateDeckWarning(float DeltaSeconds);
	void Collapse();
	void ApplyTiltRotation(const FVector2D& TiltDeg);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DeckMID;

	FVector2D CollapseDir = FVector2D(1.f, 0.f);
	float CollapseCurrent = 0.f;
	float CollapseVelocity = 0.f;
	float WarnPhase = 0.f;

	bool bGridForPlacing = false;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ASLPlaceable>> Placeables;

	float SwayTime = 0.f;
};
