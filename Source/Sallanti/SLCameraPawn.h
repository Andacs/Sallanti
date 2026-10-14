// Sabit izometrik kamera. Oyuncu dondurmez, zoom yapmaz; platform egilince kamera egilmez.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SLCameraPawn.generated.h"

class UCameraComponent;

UCLASS()
class SALLANTI_API ASLCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ASLCameraPawn();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sallanti")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (Units = "Degrees"))
	float FieldOfView = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (Units = "Degrees"))
	float Yaw = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera", meta = (Units = "Degrees"))
	float Pitch = -35.f;

	/** Bakilan noktaya (platform merkezi) uzaklik, cm. Spec 1400 diyordu; olculdu: 1400'de 600 cm'lik
	 *  platform 16:9 kadraja sigmiyor (dikeyde %153). 2200'de platform kadrajin %87'si. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	float Distance = 2200.f;

	/** Bakilan nokta, pawn'a gore. Kuleler yukari uzadigi icin biraz yukarida. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Kamera")
	FVector LookAtOffset = FVector(0.f, 0.f, 40.f);

	UFUNCTION(BlueprintCallable, Category = "Sallanti")
	void ApplyCamera();
};
