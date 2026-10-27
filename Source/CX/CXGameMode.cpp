// CXGameMode.cpp

#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
	PlayerStateClass = ACXPlayerState::StaticClass();
	GameStateClass = ACXGameState::StaticClass();
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(AvailablePlayerStarts.IsEmpty() == false, TEXT("There is no available Vehicle PlayerStart."));

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}

void ACXGameMode::InitGameState()
{
	Super::InitGameState();

	if (ACXGameState* State = Cast<ACXGameState>(GameState))
	{
		State->SetMatchStatus(ECXGamePhase::Waiting, FMath::Max(1, WaitingDuration));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Set Game State Class to CXGameState in the active GameMode Blueprint."));
	}
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsValid(NewPlayer) || bCountdownStarted)
	{
		return;
	}

	ACXGameState* State = Cast<ACXGameState>(GameState);
	if (!State)
	{
		return;
	}

	bCountdownStarted = true;
	State->SetMatchStatus(ECXGamePhase::Waiting, FMath::Max(1, WaitingDuration));
	GetWorldTimerManager().SetTimer(
		MatchTimerHandle, this, &ACXGameMode::TickMatchCountdown, 1.0f, true);
}

void ACXGameMode::TickMatchCountdown()
{
	ACXGameState* State = Cast<ACXGameState>(GameState);
	if (!State || State->GetGamePhase() == ECXGamePhase::Finished)
	{
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		return;
	}

	const int32 Remaining = FMath::Max(0, State->GetRemainingSeconds() - 1);
	if (Remaining > 0)
	{
		State->SetMatchStatus(State->GetGamePhase(), Remaining);
		return;
	}

	if (State->GetGamePhase() == ECXGamePhase::Waiting)
	{
		StartPlaying();
	}
	else
	{
		FinishMatch();
	}
}

void ACXGameMode::StartPlaying()
{
	if (ACXGameState* State = Cast<ACXGameState>(GameState))
	{
		State->SetMatchStatus(ECXGamePhase::Playing, FMath::Max(1, PlayingDuration));
	}
}

void ACXGameMode::FinishMatch()
{
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);
	if (ACXGameState* State = Cast<ACXGameState>(GameState))
	{
		State->SetMatchStatus(ECXGamePhase::Finished, 0);
		State->FinalizeResults();
	}
}

void ACXGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}


