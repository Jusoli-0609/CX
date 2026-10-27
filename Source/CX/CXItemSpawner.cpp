#include "CXItemSpawner.h"
#include "CXCoinPickup.h"
#include "CXSpeedBuffPickup.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACXItemSpawner::ACXItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SetRootComponent(SpawnPoint);
	SpawnPoint->SetHiddenInGame(true);
}

void ACXItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	// A non-replicated actor can have local authority on a client.
	// NetMode explicitly prevents clients from creating their own pickups.
	if (GetNetMode() == NM_Client || !HasAuthority())
	{
		return;
	}

	if (!ItemClass ||
		(!ItemClass->IsChildOf(ACXCoinPickup::StaticClass()) &&
		 !ItemClass->IsChildOf(ACXSpeedBuffPickup::StaticClass())))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: ItemClass must be a CoinPickup or SpeedBuffPickup Blueprint."),
			*GetName());
		return;
	}

	SpawnItem();
}

void ACXItemSpawner::SpawnItem()
{
	if (GetNetMode() == NM_Client || !HasAuthority() || bStopping ||
		!GetWorld() || !ItemClass || IsValid(SpawnedItem))
	{
		return;
	}

	const FTransform SpawnTransform = GetActorTransform();
	AActor* NewItem = GetWorld()->SpawnActorDeferred<AActor>(
		ItemClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(NewItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Item spawn failed; retrying in 2 seconds."), *GetName());
		ScheduleRespawn();
		return;
	}

	// Bind before FinishSpawning: an overlapping vehicle can collect the item immediately.
	SpawnedItem = NewItem;
	NewItem->OnDestroyed.AddUniqueDynamic(this, &ACXItemSpawner::HandleSpawnedItemDestroyed);
	UGameplayStatics::FinishSpawningActor(NewItem, SpawnTransform);
}

void ACXItemSpawner::HandleSpawnedItemDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != SpawnedItem.Get())
	{
		return;
	}

	SpawnedItem = nullptr;
	ScheduleRespawn();
}

void ACXItemSpawner::ScheduleRespawn()
{
	if (GetNetMode() == NM_Client || !HasAuthority() || bStopping || !GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		RespawnTimerHandle, this, &ACXItemSpawner::SpawnItem,
		RespawnDelaySeconds, false);
}

void ACXItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bStopping = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	if (GetNetMode() != NM_Client && IsValid(SpawnedItem))
	{
		SpawnedItem->OnDestroyed.RemoveDynamic(this, &ACXItemSpawner::HandleSpawnedItemDestroyed);
		SpawnedItem->Destroy();
	}
	SpawnedItem = nullptr;

	Super::EndPlay(EndPlayReason);
}

