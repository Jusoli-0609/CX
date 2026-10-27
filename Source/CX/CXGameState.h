#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

UENUM(BlueprintType)
enum class ECXGamePhase : uint8
{
	Waiting,
	Playing,
	Finished
};

USTRUCT(BlueprintType)
struct FCXMatchStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ECXGamePhase Phase = ECXGamePhase::Waiting;

	UPROPERTY(BlueprintReadOnly)
	int32 RemainingSeconds = 0;
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACXGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Match")
	ECXGamePhase GetGamePhase() const { return MatchStatus.Phase; }

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetRemainingSeconds() const { return MatchStatus.RemainingSeconds; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsPlaying() const { return MatchStatus.Phase == ECXGamePhase::Playing; }

	UFUNCTION(BlueprintPure, Category = "Match")
	FText GetMatchStatusText() const;

	// Called by the authoritative GameMode; never changes client state directly.
	void SetMatchStatus(ECXGamePhase NewPhase, int32 Seconds);

private:
	UPROPERTY(Replicated)
	FCXMatchStatus MatchStatus;
};

