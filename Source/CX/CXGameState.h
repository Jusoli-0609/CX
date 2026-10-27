#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class APlayerController;

UENUM(BlueprintType)
enum class ECXGamePhase : uint8
{
	Waiting,
	Playing,
	Finished
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCXGamePhaseChanged, ECXGamePhase, NewPhase);

USTRUCT(BlueprintType)
struct FCXMatchStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ECXGamePhase Phase = ECXGamePhase::Waiting;

	UPROPERTY(BlueprintReadOnly)
	int32 RemainingSeconds = 0;
};

USTRUCT(BlueprintType)
struct FCXWinnerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerId = -1;

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;
};

USTRUCT(BlueprintType)
struct FCXMatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bReady = false;

	UPROPERTY(BlueprintReadOnly)
	int32 WinningCoins = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<FCXWinnerInfo> Winners;
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

	UFUNCTION(BlueprintPure, Category = "Match")
	FText GetMatchResultText(APlayerController* Viewer) const;

	UFUNCTION(BlueprintPure, Category = "Match")
	TArray<FCXWinnerInfo> GetWinners() const { return MatchResult.Winners; }

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FCXGamePhaseChanged OnGamePhaseChanged;

	void SetMatchStatus(ECXGamePhase NewPhase, int32 Seconds);
	void FinalizeResults();

private:
	UPROPERTY(ReplicatedUsing = OnRep_MatchStatus)
	FCXMatchStatus MatchStatus;

	UPROPERTY(Replicated)
	FCXMatchResult MatchResult;

	UFUNCTION()
	void OnRep_MatchStatus();
};

