#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"

ACXPlayerState::ACXPlayerState()
{
	bReplicates = true;
}

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACXPlayerState, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACXPlayerState, BonusMaxSpeed, COND_OwnerOnly);
}

bool ACXPlayerState::AddCoins(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0 || Coins > MAX_int32 - Amount)
	{
		return false;
	}

	Coins += Amount;
	ForceNetUpdate();
	return true;
}

bool ACXPlayerState::TrySpendCoins(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0 || Coins < Amount)
	{
		return false;
	}

	Coins -= Amount;
	ForceNetUpdate();
	return true;
}

bool ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (!HasAuthority() || !FMath::IsFinite(Amount) || Amount <= 0.0f)
	{
		return false;
	}

	const float NewSpeed = BonusMaxSpeed + Amount;
	if (!FMath::IsFinite(NewSpeed))
	{
		return false;
	}

	BonusMaxSpeed = NewSpeed;
	ForceNetUpdate();
	return true;
}

FText ACXPlayerState::GetStatsText() const
{
	return FText::Format(
		NSLOCTEXT("CX", "PlayerStatsSummary", "코인: {0}\n추가 최대 속도: +{1}"),
		FText::AsNumber(Coins),
		FText::AsNumber(BonusMaxSpeed));
}

