#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ACXPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Player Stats")
	int32 GetCoins() const { return Coins; }

	UFUNCTION(BlueprintPure, Category = "Player Stats")
	float GetBonusMaxSpeed() const { return BonusMaxSpeed; }

	UFUNCTION(BlueprintPure, Category = "Player Stats")
	FText GetStatsText() const;

	// These are ordinary functions, not RPCs. Call them on the server.
	UFUNCTION(BlueprintCallable, Category = "Player Stats")
	bool AddCoins(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Player Stats")
	bool TrySpendCoins(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Player Stats")
	bool AddBonusMaxSpeed(float Amount);

private:
	bool CanChangeStats() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Player Stats",
		meta = (AllowPrivateAccess = "true"))
	int32 Coins = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Player Stats",
		meta = (AllowPrivateAccess = "true"))
	float BonusMaxSpeed = 0.0f;
};


