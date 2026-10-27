#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "CXItemSpawner.generated.h"

class UArrowComponent;

UCLASS()
class CX_API ACXItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACXItemSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UArrowComponent> SpawnPoint;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<AActor> ItemClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<AActor> SpawnedItem;

private:
	static constexpr float RespawnDelaySeconds = 2.0f;

	FTimerHandle RespawnTimerHandle;
	bool bStopping = false;

	void SpawnItem();
	void ScheduleRespawn();

	UFUNCTION()
	void HandleSpawnedItemDestroyed(AActor* DestroyedActor);
};

