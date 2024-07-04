/*
Copyright 2022 ATMTA, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Author: Daniele Calanna
Contributers: Riccardo Torrisi, Federico Arona
*/
#pragma once
#include "CoreMinimal.h"
#include "Network/UGI_WebSocketManager.h"
#include "SolanaWallet.h"
#include "SolanaUtils/Account.h"
#include "SolanaUtils/Utils/Types.h"
#include "WalletAccount.generated.h"

class UTokenAccount;

DECLARE_DELEGATE_TwoParams(FJoinBattleDelegate, bool, FString&);

/**
 * UWalletAccount
 * 
 * This class abstract a wallet account which is essentially a key pair.
 * We can have either a public key only or both private and public key.
 * The public key of the account represent the address in the network.
 * An account also contains a list of token accounts associated with its keypair.
 * 
 */
UCLASS(BlueprintType)
class FOUNDATION_API UWalletAccount : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FAccount AccountData;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, UTokenAccount*> TokenAccounts;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTokenAccountAdded, UWalletAccount*, WalletAccount, UTokenAccount*,
	                                             TokenAccount);

	UPROPERTY(BlueprintAssignable)
	FOnTokenAccountAdded OnTokenAccountAdded;

	DECLARE_EVENT(UWalletAccount, FOnTokenAccountReceived);

	FOnTokenAccountReceived OnTokenAccountReceived;

	UFUNCTION(BlueprintPure)
	FString GetAccountName() const { return AccountData.Name; }

	UFUNCTION(BlueprintPure)
	FString GetPublicKey() const { return AccountData.PublicKey; }

	UFUNCTION(BlueprintCallable)
	void SetAccountName(const FString& Name);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAccountNameChanged, UWalletAccount*, WalletAccount, FString, Name);

	UPROPERTY(BlueprintAssignable)
	FOnAccountNameChanged OnAccountNameChanged;

	TOptional<double> Lamports;

	bool HasLamportBeenSet() const { return Lamports.IsSet(); }

	UFUNCTION(BlueprintPure)
	double GetSolBalance() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSolBalanceChanged, UWalletAccount*, WalletAccount, float,
	                                             SolBalance);

	UPROPERTY(BlueprintAssignable)
	FOnSolBalanceChanged OnSolBalanceChanged;

	UFUNCTION(BlueprintCallable)
	void Update();

	void UpdateData();
	void UpdateTokenAccounts();

	void UpdateFromAccountInfoJson(const FAccountInfoJson& AccountInfoJson);

	// TODO support UTokenAccount
	void SendSOL(const FAccount& From, const FAccount& To, int64 Amount) const;
	void SendSOLEstimate(const FAccount& From, const FAccount& To, int64 Amount) const;

	UFUNCTION(BlueprintCallable)
	void SendToken(UTokenAccount* TokenAccount, const FString& RecipientPublicKey, float Amount);

	UFUNCTION(BlueprintCallable)
	void SendTokenEstimate(UTokenAccount* TokenAccount, const FString& RecipientPublicKey, float Amount) const;

	UFUNCTION(BlueprintCallable)
	void JoinBattle(const FAccount& User, int32 Collateral) const;
	FJoinBattleDelegate OnJoinBattle;

	void Sub2AccountInfo(const FString& PubKey, UGI_WebSocketManager* & SocketManager);
	void UnSub2AccountInfo(const int& SubID, UGI_WebSocketManager* & SocketManager);
	double ReadSub(int ID, UGI_WebSocketManager*& SocketManager);

	UFUNCTION(BlueprintPure)
	USolanaWallet* GetOwningWallet() const { return CastChecked<USolanaWallet>(GetOuter()); }
};
