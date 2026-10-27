#include "CXCoinPickup.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ACXCoinPickup::ACXCoinPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	NetUpdateFrequency = 1.0f;
	MinNetUpdateFrequency = 1.0f;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);
	PickupSphere->InitSphereRadius(100.0f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ACXCoinPickup::HandleBeginOverlap);

	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	CoinMesh->SetupAttachment(PickupSphere);
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoinMesh->SetGenerateOverlapEvents(false);
	CoinMesh->SetRelativeScale3D(FVector(0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CoinMesh->SetStaticMesh(SphereMesh.Object);
	}
}

void ACXCoinPickup::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bCollected)
	{
		return;
	}

	ACXPawn* Vehicle = Cast<ACXPawn>(OtherActor);
	if (!Vehicle)
	{
		return;
	}

	ACXPlayerState* State = Vehicle->GetPlayerState<ACXPlayerState>();
	if (!State)
	{
		return;
	}

	// Claim the item before applying the reward, including re-entrant overlaps.
	bCollected = true;
	if (!State->AddCoins(10))
	{
		bCollected = false;
		return;
	}

	PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MulticastPlayPickupEffects(GetActorLocation());
	Destroy();
}

void ACXCoinPickup::MulticastPlayPickupEffects_Implementation(FVector Location)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (PickupNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), PickupNiagara, Location, FRotator::ZeroRotator);
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, Location);
	}
}




void ACXCoinPickup::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	MatchGameState = GetWorld() ? GetWorld()->GetGameState<ACXGameState>() : nullptr;
	if (!MatchGameState)
	{
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogTemp, Warning, TEXT("%s: CXGameState is required for pickup collection."), *GetName());
		return;
	}

	MatchGameState->OnGamePhaseChanged.AddUniqueDynamic(this, &ACXCoinPickup::HandleGamePhaseChanged);
	HandleGamePhaseChanged(MatchGameState->GetGamePhase());
}

void ACXCoinPickup::HandleGamePhaseChanged(ECXGamePhase NewPhase)
{
	if (!HasAuthority() || bCollected)
	{
		return;
	}

	PickupSphere->SetCollisionEnabled(NewPhase == ECXGamePhase::Playing
		? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void ACXCoinPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(MatchGameState))
	{
		MatchGameState->OnGamePhaseChanged.RemoveDynamic(this, &ACXCoinPickup::HandleGamePhaseChanged);
	}
	MatchGameState = nullptr;
	Super::EndPlay(EndPlayReason);
}

