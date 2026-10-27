#include "CXGameState.h"
#include "Net/UnrealNetwork.h"
#include "CXPlayerState.h"
#include "GameFramework/PlayerController.h"

ACXGameState::ACXGameState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 10.0f;
}

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACXGameState, MatchStatus);
	DOREPLIFETIME(ACXGameState, MatchResult);
}

void ACXGameState::SetMatchStatus(ECXGamePhase NewPhase, int32 Seconds)
{
	if (!HasAuthority())
	{
		return;
	}

	const ECXGamePhase OldPhase = MatchStatus.Phase;
	MatchStatus.Phase = NewPhase;
	MatchStatus.RemainingSeconds = FMath::Max(0, Seconds);
	ForceNetUpdate();

	if (OldPhase != NewPhase)
	{
		OnGamePhaseChanged.Broadcast(NewPhase);
	}
}

FText ACXGameState::GetMatchStatusText() const
{
	FText PhaseText;
	switch (MatchStatus.Phase)
	{
	case ECXGamePhase::Waiting:
		PhaseText = NSLOCTEXT("CX", "PhaseWaiting", "대기");
		break;
	case ECXGamePhase::Playing:
		PhaseText = NSLOCTEXT("CX", "PhasePlaying", "진행");
		break;
	case ECXGamePhase::Finished:
		PhaseText = NSLOCTEXT("CX", "PhaseFinished", "종료");
		break;
	default:
		PhaseText = FText::GetEmpty();
		break;
	}

	return FText::Format(
		NSLOCTEXT("CX", "MatchStatusSummary", "상태: {0}\n남은 시간: {1}초"),
		PhaseText, FText::AsNumber(MatchStatus.RemainingSeconds));
}



void ACXGameState::OnRep_MatchStatus()
{
	OnGamePhaseChanged.Broadcast(MatchStatus.Phase);
}

void ACXGameState::FinalizeResults()
{
	if (!HasAuthority() || MatchStatus.Phase != ECXGamePhase::Finished || MatchResult.bReady)
	{
		return;
	}

	FCXMatchResult FinalResult;
	int32 BestCoins = -1;
	for (APlayerState* Entry : PlayerArray)
	{
		const ACXPlayerState* State = Cast<ACXPlayerState>(Entry);
		if (!IsValid(State) || !IsValid(State->GetPlayerController()) || State->IsOnlyASpectator())
		{
			continue;
		}

		const int32 Coins = State->GetCoins();
		if (Coins > BestCoins)
		{
			BestCoins = Coins;
			FinalResult.Winners.Reset();
		}
		if (Coins == BestCoins)
		{
			FCXWinnerInfo Winner;
			Winner.PlayerId = State->GetPlayerId();
			Winner.PlayerName = State->GetPlayerName().IsEmpty()
				? FString::Printf(TEXT("Player %d"), Winner.PlayerId)
				: State->GetPlayerName();
			FinalResult.Winners.Add(Winner);
		}
	}

	FinalResult.WinningCoins = FMath::Max(0, BestCoins);
	FinalResult.bReady = true;
	MatchResult = MoveTemp(FinalResult);
	ForceNetUpdate();

	UE_LOG(LogTemp, Log, TEXT("Match result frozen: highest coins=%d, winners=%d"),
		MatchResult.WinningCoins, MatchResult.Winners.Num());
}

FText ACXGameState::GetMatchResultText(APlayerController* Viewer) const
{
	if (MatchStatus.Phase != ECXGamePhase::Finished)
	{
		return FText::GetEmpty();
	}
	if (!MatchResult.bReady)
	{
		return NSLOCTEXT("CX", "ResultPending", "결과 집계 중");
	}
	if (MatchResult.Winners.IsEmpty())
	{
		return NSLOCTEXT("CX", "NoParticipants", "경기 종료\n참가자 없음");
	}

	const APlayerState* ViewerState = Viewer ? Viewer->GetPlayerState<APlayerState>() : nullptr;
	if (!ViewerState)
	{
		return NSLOCTEXT("CX", "ViewerPending", "플레이어 정보 준비 중");
	}

	bool bWinner = false;
	TArray<FString> WinnerNames;
	for (const FCXWinnerInfo& Winner : MatchResult.Winners)
	{
		WinnerNames.Add(Winner.PlayerName);
		bWinner |= Winner.PlayerId == ViewerState->GetPlayerId();
	}

	FText Outcome = NSLOCTEXT("CX", "Defeat", "패배");
	if (bWinner)
	{
		Outcome = MatchResult.Winners.Num() > 1
			? NSLOCTEXT("CX", "JointVictory", "공동 승리!")
			: NSLOCTEXT("CX", "Victory", "승리!");
	}

	return FText::Format(
		NSLOCTEXT("CX", "FinalResultSummary", "{0}\n우승자: {1}\n최고 코인: {2}"),
		Outcome, FText::FromString(FString::Join(WinnerNames, TEXT(", "))),
		FText::AsNumber(MatchResult.WinningCoins));
}

