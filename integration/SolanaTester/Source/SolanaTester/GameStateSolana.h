// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Solana/PublicKey.h"
#include "GameStateSolana.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FMoveLeftCompleteDelegate, bool, Success);
DECLARE_DYNAMIC_DELEGATE_OneParam(FMoveRightCompleteDelegate, bool, Success);
DECLARE_DYNAMIC_DELEGATE(FWalletIsLockedDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FGameInitializationCompleteDelegate, bool, Success);

/**
 * 
 */
UCLASS()
class SOLANATESTER_API AGameStateSolana : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGameStateSolana();

	UFUNCTION(BlueprintCallable)
	UWalletAccount* EnsureUnlockedWallet(const FWalletIsLockedDelegate& OnWalletLocked);
	
	UFUNCTION(BlueprintCallable)
	void InitializeGame(const FGameInitializationCompleteDelegate OnGameInitializationComplete, const FWalletIsLockedDelegate& OnWalletLocked);
	
	UFUNCTION(BlueprintCallable)
	void SendMoveLeftTransaction(const FMoveLeftCompleteDelegate OnMoveLeftComplete, const FWalletIsLockedDelegate& OnWalletLocked);
	
	UFUNCTION(BlueprintCallable)
	void SendMoveRightTransaction(const FMoveRightCompleteDelegate OnMoveRightComplete, const FWalletIsLockedDelegate& OnWalletLocked);

private:
	FPublicKey GameDataAccount; 
};
