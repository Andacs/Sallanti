// Bolum basinda platform kenarinda bekleyen aile. Gorevleri konusma balonuyla bu aile verir.
// Ev kurulunca bu figurler gizlenir ve ayni noktadan evin kendi figurleri yuruyerek kapiya gider
// (ASLPlaceable::StartResidentsArrival): oyuncu tek bir ailenin yurudugunu gorur.
// Yapay zeka degil: duran figurler. Kesme listesindeki "NPC/AI yok" karariyla celismez.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SLFamily.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class SALLANTI_API ASLFamily : public AActor
{
	GENERATED_BODY()

public:
	ASLFamily();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	int32 Members = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	float HeightCm = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Aile")
	FLinearColor Tint = FLinearColor(0.15f, 0.35f, 0.75f);

	/** Figurleri kurar (platform TiltPivot'una bagli dogduktan sonra). */
	void BuildFigures();

	/** Konusma balonunun baglanacagi dunya noktasi: figurlerin basinin ustu. */
	FVector GetSpeechAnchor() const;

	void SetWaitingVisible(bool bVisible);
	bool IsWaitingVisible() const { return bWaitingVisible; }

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Figures;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cube;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMat;

	bool bWaitingVisible = true;
};
