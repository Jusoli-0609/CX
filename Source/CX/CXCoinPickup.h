#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXCoinPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;
class UPrimitiveComponent;

UCLASS()
class CX_API ACXCoinPickup : public AActor
{
	GENERATED_BODY()

public:
	ACXCoinPickup();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	TObjectPtr<UStaticMeshComponent> CoinMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin|Effects")
	TObjectPtr<UParticleSystem> PickupParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin|Effects")
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

