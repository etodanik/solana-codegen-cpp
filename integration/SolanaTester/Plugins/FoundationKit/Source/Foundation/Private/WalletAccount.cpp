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

#include "WalletAccount.h"
#include "JsonObjectConverter.h"
#include "TokenAccount.h"
#include "Network/RequestManager.h"
#include "Network/RequestUtils.h"
#include "Network/SubscriptionUtils.h"

void UWalletAccount::SetAccountName(const FString& Name)
{
	if (AccountData.Name != Name)
	{
		AccountData.Name = Name;
		OnAccountNameChanged.Broadcast(this, Name);
	}
}

void UWalletAccount::Update()
{
	UpdateData();
	UpdateTokenAccounts();
}

void UWalletAccount::UpdateData()
{
	const auto Request = FRequestUtils::RequestAccountInfo(AccountData.PublicKey, ERequestEncoding::Base58);
	Request->Callback.BindLambda([this](FJsonObject& Data)
	{
		const FAccountInfoJson response = FRequestUtils::ParseAccountInfoResponse(Data);
		UpdateFromAccountInfoJson(response);
	});
	FRequestManager::SendRequest(Request);
}

void UWalletAccount::UpdateTokenAccounts()
{
	const auto Request = FRequestUtils::RequestAllTokenAccounts(AccountData.PublicKey,
	                                                            "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA");
	Request->Callback.BindLambda([this](FJsonObject& Data)
	{
		TokenAccounts.Empty();
		if (const TSharedPtr<FJsonObject> result = Data.GetObjectField("result"))
		{
			FTokenAccountArrayJson jsonData;
			FJsonObjectConverter::JsonObjectToUStruct(result.ToSharedRef(), &jsonData);

			if (!jsonData.value.IsEmpty())
			{
				for (FTokenBalanceDataJson entry : jsonData.value)
				{
					FTokenInfoJson Info = entry.account.data.parsed.info;

					UTokenAccount* TokenAccount = TokenAccounts.Contains(Info.mint)
						                              ? TokenAccounts[Info.mint]
						                              : NewObject<UTokenAccount>(this);
					TokenAccounts.Add(Info.mint, TokenAccount);
					FAccountData& account = TokenAccount->AccountData;
					account.Pubkey = entry.pubkey;
					account.Balance = Info.tokenAmount.uiAmount;
					account.Mint = Info.mint;
					TokenAccount->OnBalanceUpdated.Broadcast(TokenAccount, account.Balance);
					OnTokenAccountAdded.Broadcast(this, TokenAccount);
				}
			}

			OnTokenAccountReceived.Broadcast();
		}
	});
	FRequestManager::SendRequest(Request);
}

void UWalletAccount::UpdateFromAccountInfoJson(const FAccountInfoJson& AccountInfoJson)
{
	Lamports = AccountInfoJson.lamports;
	OnSolBalanceChanged.Broadcast(this, GetSolBalance());
}

void UWalletAccount::SendSOL(const FAccount& From, const FAccount& To, int64 Amount) const
{
	// const auto Request = FRequestUtils::RequestBlockHash();
	// Request->Callback.BindLambda([this, From, To, Amount](FJsonObject& Data)
	// {
	// 	const FString BlockHash = FRequestUtils::ParseBlockHashResponse(Data);
	//
	// 	const TArray<uint8> TransferSolTransactoin = FTransactionUtils::TransferSOLTransaction(
	// 		From, To, Amount, BlockHash);
	//
	// 	const auto Transaction = FRequestUtils::SendTransaction(FBase64::Encode(TransferSolTransactoin));
	// 	Transaction->Callback.BindLambda([this, From, To, Amount](FJsonObject& Data)
	// 	{
	// 		const FString TransactionId = FRequestUtils::ParseTransactionResponse(Data);
	// 		FRequestUtils::DisplayInfo(FString::Printf(TEXT("Transaction Id: %s"), *TransactionId));
	// 	});
	// 	FRequestManager::SendRequest(Transaction);
	// });
	// FRequestManager::SendRequest(Request);
}

void UWalletAccount::SendSOLEstimate(const FAccount& from, const FAccount& To, int64 Amount) const
{
	// const auto Request = FRequestUtils::RequestBlockHash();
	// Request->Callback.BindLambda([this, from, To, Amount](FJsonObject& Data)
	// {
	// 	const FString BlockHash = FRequestUtils::ParseBlockHashResponse(Data);
	//
	// 	const TArray<uint8> transaction = FTransactionUtils::TransferSOLTransaction(from, To, Amount, BlockHash);
	//
	// 	const auto feeRequest = FRequestUtils::GetTransactionFeeAmount(FBase64::Encode(transaction));
	// 	feeRequest->Callback.BindLambda([this, from, To, Amount](FJsonObject& Data)
	// 	{
	// 		const int fee = FRequestUtils::ParseTransactionFeeAmountResponse(Data);
	// 		FRequestUtils::DisplayInfo(FString::Printf(TEXT("Estimate Id: %i"), fee));
	// 	});
	// 	FRequestManager::SendRequest(feeRequest);
	// });
	// FRequestManager::SendRequest(Request);
}

void UWalletAccount::SendToken(UTokenAccount* TokenAccount, const FString& RecipientPublicKey,
                               float Amount)
{
	// UE_LOG(LogTemp, Warning, TEXT("SendToken function"))
	// const FAccountData& TokenAccountData = TokenAccount->AccountData;
	// FAccount RecipientAccount = FAccount::FromPublicKey(RecipientPublicKey);
	//
	// const auto AccountRequest = FRequestUtils::RequestTokenAccount(AccountData.PublicKey, TokenAccountData.Mint);
	// AccountRequest->Callback.BindLambda([this, TokenAccountData, RecipientAccount, Amount](FJsonObject& Data)
	// {
	// 	FString existingAccount = FRequestUtils::ParseTokenAccountResponse(Data);
	// 	if (!existingAccount.IsEmpty())
	// 	{
	// 		const auto blockhashRequest = FRequestUtils::RequestBlockHash();
	// 		blockhashRequest->Callback.BindLambda(
	// 			[this, TokenAccountData, RecipientAccount, Amount, existingAccount](FJsonObject& Data)
	// 			{
	// 				const FString BlockHash = FRequestUtils::ParseBlockHashResponse(Data);
	//
	// 				const TArray<uint8> transaction = FTransactionUtils::TransferTokenTransaction(
	// 					AccountData, RecipientAccount, AccountData, Amount, TokenAccountData.Mint, BlockHash,
	// 					existingAccount);
	//
	// 				const auto sendTransaction = FRequestUtils::SendTransaction(FBase64::Encode(transaction));
	// 				sendTransaction->Callback.BindLambda([this](FJsonObject& Data)
	// 				{
	// 					const FString TransactionId = FRequestUtils::ParseTransactionResponse(Data);
	// 					FRequestUtils::DisplayInfo(FString::Printf(TEXT("Transaction Id: %s"), *TransactionId));
	// 				});
	// 				FRequestManager::SendRequest(sendTransaction);
	// 			});
	// 		FRequestManager::SendRequest(blockhashRequest);
	// 	}
	// });
	// FRequestManager::SendRequest(AccountRequest);
}

void UWalletAccount::SendTokenEstimate(UTokenAccount* TokenAccount, const FString& RecipientPublicKey,
                                       float Amount) const
{
	// const FAccountData& TokenAccountData = TokenAccount->AccountData;
	// FAccount RecipientAccount = FAccount::FromPublicKey(RecipientPublicKey);
	//
	// const auto AccountRequest = FRequestUtils::RequestTokenAccount(TokenAccountData.Pubkey, TokenAccountData.Mint);
	// AccountRequest->Callback.BindLambda([this, TokenAccountData, RecipientAccount, Amount](FJsonObject& Data)
	// {
	// 	FString existingAccount = FRequestUtils::ParseTokenAccountResponse(Data);
	// 	if (!existingAccount.IsEmpty())
	// 	{
	// 		const auto blockhashRequest = FRequestUtils::RequestBlockHash();
	// 		blockhashRequest->Callback.BindLambda(
	// 			[this, TokenAccountData, RecipientAccount, Amount, existingAccount](FJsonObject& Data)
	// 			{
	// 				const FString BlockHash = FRequestUtils::ParseBlockHashResponse(Data);
	// 				const TArray<uint8> transaction = FTransactionUtils::TransferTokenTransaction(
	// 					AccountData, RecipientAccount, AccountData, Amount, TokenAccountData.Mint, BlockHash,
	// 					existingAccount);
	//
	// 				const auto sendTransaction =
	// 					FRequestUtils::GetTransactionFeeAmount(FBase64::Encode(transaction));
	// 				sendTransaction->Callback.BindLambda([this](FJsonObject& Data)
	// 				{
	// 					const int fee = FRequestUtils::ParseTransactionFeeAmountResponse(Data);
	// 					FRequestUtils::DisplayInfo(FString::Printf(TEXT("Estimate Id: %i"), fee));
	// 				});
	// 				FRequestManager::SendRequest(sendTransaction);
	// 			});
	// 		FRequestManager::SendRequest(blockhashRequest);
	// 	}
	// });
	// FRequestManager::SendRequest(AccountRequest);
}

double UWalletAccount::GetSolBalance() const
{
	return Lamports.Get(0.f) / 1e9;
}

void UWalletAccount::Sub2AccountInfo(const FString& pubKey, UGI_WebSocketManager* & SocketManager)
{
	SocketManager->InitializeHeartbeat();
	FSubscriptionData* SubRequest = FSubscriptionUtils::AccountSubscribe(pubKey);
	SocketManager->Subscribe(SubRequest);
}

void UWalletAccount::UnSub2AccountInfo(const int& ID2Remove, UGI_WebSocketManager*& SocketManager)
{
	FSubscriptionUtils::AccountUnsubscribe(SocketManager->ActiveSubscriptions[ID2Remove]);
	SocketManager->Unsubscribe(ID2Remove);
}

double UWalletAccount::ReadSub(int ID, UGI_WebSocketManager*& SocketManager)
{
	FSubscriptionData* subData = SocketManager->GetSubData(ID);
	return FSubscriptionUtils::GetAccountSubInfo(subData);
}

void UWalletAccount::JoinBattle(const FAccount& User, int32 Collateral) const
{
	// const auto Request = FRequestUtils::RequestBlockHash();
	// Request->Callback.BindLambda([this, User, Collateral](FJsonObject& Data)
	// {
	// 	const FString BlockHash = FRequestUtils::ParseBlockHashResponse(Data);
	//
	// 	const TArray<uint8> TransactionData = FTransactionUtils::JoinBattleTransaction(User, Collateral, BlockHash);
	//
	// 	const auto Transaction = FRequestUtils::SendTransaction(FBase64::Encode(TransactionData));
	// 	Transaction->Callback.BindLambda([this, User, Collateral](FJsonObject& Data)
	// 	{
	// 		FString EmptyString("");
	// 		// OnJoinBattle.ExecuteIfBound(true, EmptyString);
	// 		const FString TransactionId = FRequestUtils::ParseTransactionResponse(Data);
	// 		FRequestUtils::DisplayInfo(FString::Printf(TEXT("Transaction Id: %s"), *TransactionId));
	// 		if (TransactionId.IsEmpty())
	// 		{
	// 			UE_LOG(LogTemp, Error, TEXT("JoinBattle did not return a tx id"));
	// 			return;
	// 		}
	//
	// 		UE_LOG(LogTemp, Error, TEXT("JoinBattle tx id: %s"), *TransactionId);
	// 	});
	//
	// 	Transaction->ErrorCallback.BindLambda([this](FString& Error)
	// 	{
	// 		OnJoinBattle.ExecuteIfBound(false, Error);
	// 	});
	//
	// 	FRequestManager::SendRequest(Transaction);
	// });
	//
	// FRequestManager::SendRequest(Request);
}
