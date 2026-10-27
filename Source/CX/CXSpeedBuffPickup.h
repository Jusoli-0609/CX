#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXSpeedBuffPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;
class UPrimitiveComponent;

UCLASS()
class CX_API ACXSpeedBuffPickup : public AActor
{
	GENERATED_BODY()

public:
	ACXSpeedBuffPickup();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed Buff")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed Buff")
	TObjectPtr<UStaticMeshComponent> BuffMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Speed Buff|Effects")
	TObjectPtr<UNiagaraSystem> PickupNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Speed Buff|Effects")
	TObjectPtr<USoundBase> PickupSound;

private:
	bool bCollected = false;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayPickupEffects(FVector Location);
};



