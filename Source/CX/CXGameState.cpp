#include "CXGameState.h"
#include "Net/UnrealNetwork.h"

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
}

void ACXGameState::SetMatchStatus(ECXGamePhase NewPhase, int32 Seconds)
{
	if (!HasAuthority())
	{
		return;
	}

	MatchStatus.Phase = NewPhase;
	MatchStatus.RemainingSeconds = FMath::Max(0, Seconds);
	ForceNetUpdate();
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

