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

Author: Jon Sawler
Contributers: Daniele Calanna, Federico Arona
*/

#include "SolanaUtils/Wallet.h"

#include "Network/RequestManager.h"
#include "JsonObjectConverter.h"
#include "Network/RequestUtils.h"
#include "SolanaUtils/Utils/Types.h"

UWallet::UWallet()
{
}

UWallet::~UWallet()
{
	TokenAccounts.Empty();
}

void UWallet::SetPublicKey(const FString& pubKey)
{
	PublicKey = pubKey;
	Account.PublicKey = pubKey;
}

bool UWallet::IsValidPublicKey(const FString& pubKey)
{
	return pubKey.Len() > 0;
}

bool UWallet::IsValidPublicKey(const TArray<FString>& pubKeys)
{
	for (FString key : pubKeys)
	{
		if (!IsValidPublicKey(key))
		{
			return false;
		}
	}
	return true;
}

const FAccountData* UWallet::GetAccountByMint(const FString& mint)
{
	return TokenAccounts.FindByPredicate([mint](const FAccountData data) { return data.Mint == mint; });
}

void UWallet::UpdateWalletData()
{
	if (IsValidPublicKey(PublicKey))
	{
		const auto Request = FRequestUtils::RequestAccountInfo(PublicKey, ERequestEncoding::Base58);
		Request->Callback.BindLambda([this](const FJsonObject& data)
		{
			const FAccountInfoJson response = FRequestUtils::ParseAccountInfoResponse(data);
			//AccountData->data = response.data.data;
			//AccountData->encoding = response.data.encoding;
			SOLBalance = response.lamports / 10;

			OnWalletUpdated.Broadcast(this);
		});
		FRequestManager::SendRequest(Request);
	}
}

void UWallet::UpdateWalletBalance()
{
	if (IsValidPublicKey(PublicKey))
	{
		const auto Request = FRequestUtils::RequestAccountBalance(PublicKey);
		Request->Callback.BindLambda([this](const FJsonObject& data)
		{
			SOLBalance = FRequestUtils::ParseAccountBalanceResponse(data);

			OnWalletUpdated.Broadcast(this);
		});
		FRequestManager::SendRequest(Request);
	}
}

void UWallet::UpdateAccountBalance(const FString& pubKey)
{
	if (IsValidPublicKey(pubKey))
	{
		const auto Request = FRequestUtils::RequestAccountBalance(pubKey);
		Request->Callback.BindLambda([this, pubKey](const FJsonObject& data)
		{
			const double balance = FRequestUtils::ParseAccountBalanceResponse(data);

			FAccountData* accountData = TokenAccounts.FindByPredicate([this, pubKey](FAccountData account)
			{
				return account.Pubkey == pubKey;
			});
			if (accountData)
			{
				accountData->Balance = balance;
			}
		});
		FRequestManager::SendRequest(Request);
	}
}

void UWallet::UpdateAllTokenAccounts()
{
	if (IsValidPublicKey(PublicKey))
	{
		const auto Request = FRequestUtils::RequestAllTokenAccounts(PublicKey, TokenProgramId);
		Request->Callback.BindLambda([this](const FJsonObject& data)
		{
			TokenAccounts.Empty();
			if (const TSharedPtr<FJsonObject> result = data.GetObjectField("result"))
			{
				FTokenAccountArrayJson jsonData;
				FJsonObjectConverter::JsonObjectToUStruct(result.ToSharedRef(), &jsonData);

				if (!jsonData.value.IsEmpty())
				{
					for (FTokenBalanceDataJson entry : jsonData.value)
					{
						FTokenInfoJson info = entry.account.data.parsed.info;

						FAccountData account;
						account.Pubkey = entry.pubkey;
						account.Balance = info.tokenAmount.uiAmount;
						account.Mint = info.mint;

						TokenAccounts.Add(account);
					}
					OnAccountsUpdated.Broadcast(this);
				}
			}
		});
		FRequestManager::SendRequest(Request);
	}
}

void UWallet::CheckPossibleAccounts(const TArray<FString>& pubKeys)
{
	if (IsValidPublicKey(pubKeys))
	{
		const auto Request = FRequestUtils::RequestMultipleAccounts(pubKeys);
		Request->Callback.BindLambda([this](const FJsonObject& data)
		{
			const TArray<FAccountInfoJson> response = FRequestUtils::ParseMultipleAccountsResponse(data);
			//TODO: stuff?			
		});
		FRequestManager::SendRequest(Request);
	}
}

void UWallet::SendSOL(const FAccount& From, const FAccount& to, int64 amount) const
{
	// const auto Request = FRequestUtils::RequestBlockHash();
	// Request->Callback.BindLambda([this, From, to, amount](const FJsonObject& data)
	// {
	// 	const FString blockHash = FRequestUtils::ParseBlockHashResponse(data);
	//
	// 	const TArray<uint8> TransferSolTransaction = FTransactionUtils::TransferSOLTransaction(
	// 		From, to, amount, blockHash);
	// 	const auto Transaction = FRequestUtils::SendTransaction(FBase64::Encode(TransferSolTransaction));
	// 	Transaction->Callback.BindLambda([this, From, to, amount](const FJsonObject& data)
	// 	{
	// 		const FString TransactionID = FRequestUtils::ParseTransactionResponse(data);
	// 		FRequestUtils::DisplayInfo(FString::Printf(TEXT("Transaction Id: %s"), *TransactionID));
	// 	});
	// 	FRequestManager::SendRequest(Transaction);
	// });
	// FRequestManager::SendRequest(Request);
}

void UWallet::SendSOLEstimate(const FAccount& From, const FAccount& to, int64 amount) const
{
	// const auto Request = FRequestUtils::RequestBlockHash();
	// Request->Callback.BindLambda([this, From, to, amount](const FJsonObject& data)
	// {
	// 	const FString blockHash = FRequestUtils::ParseBlockHashResponse(data);
	//
	// 	const TArray<uint8> Transaction = FTransactionUtils::TransferSOLTransaction(From, to, amount, blockHash);
	//
	// 	const auto feeRequest = FRequestUtils::GetTransactionFeeAmount(FBase64::Encode(Transaction));
	// 	feeRequest->Callback.BindLambda([this, From, to, amount](const FJsonObject& data)
	// 	{
	// 		const int fee = FRequestUtils::ParseTransactionFeeAmountResponse(data);
	// 		FRequestUtils::DisplayInfo(FString::Printf(TEXT("Estimate Id: %i"), fee));
	// 	});
	// 	FRequestManager::SendRequest(feeRequest);
	// });
	// FRequestManager::SendRequest(Request);
}

void UWallet::SendTokenEstimate(const FAccount& From, const FAccount& to, const FString& mint, int64 amount) const
{
	// const auto accountRequest = FRequestUtils::RequestTokenAccount(to.PublicKey, mint);
	// accountRequest->Callback.BindLambda([this, From, to, amount, mint](const FJsonObject& data)
	// {
	// 	FString existingAccount = FRequestUtils::ParseTokenAccountResponse(data);
	// 	if (!existingAccount.IsEmpty())
	// 	{
	// 		const auto BlockhashRequest = FRequestUtils::RequestBlockHash();
	// 		BlockhashRequest->Callback.BindLambda(
	// 			[this, From, to, amount, mint, existingAccount](const FJsonObject& data)
	// 			{
	// 				const FString blockHash = FRequestUtils::ParseBlockHashResponse(data);
	//
	// 				const TArray<uint8> Transaction = FTransactionUtils::TransferTokenTransaction(
	// 					From, to, Account, amount, mint, blockHash, existingAccount);
	//
	// 				const auto sendTransaction =
	// 					FRequestUtils::GetTransactionFeeAmount(FBase64::Encode(Transaction));
	// 				sendTransaction->Callback.BindLambda([this](FJsonObject& data)
	// 				{
	// 					const int fee = FRequestUtils::ParseTransactionFeeAmountResponse(data);
	// 					FRequestUtils::DisplayInfo(FString::Printf(TEXT("Estimate Id: %i"), fee));
	// 				});
	// 				FRequestManager::SendRequest(sendTransaction);
	// 			});
	// 		FRequestManager::SendRequest(BlockhashRequest);
	// 	}
	// });
	// FRequestManager::SendRequest(accountRequest);
}

void UWallet::SendToken(const FAccount& From, const FAccount& to, const FString& mint, int64 amount) const
{
	// const auto accountRequest = FRequestUtils::RequestTokenAccount(to.PublicKey, mint);
	// accountRequest->Callback.BindLambda([this, From, to, amount, mint](const FJsonObject& data)
	// {
	// 	FString existingAccount = FRequestUtils::ParseTokenAccountResponse(data);
	// 	if (!existingAccount.IsEmpty())
	// 	{
	// 		const auto BlockhashRequest = FRequestUtils::RequestBlockHash();
	// 		BlockhashRequest->Callback.BindLambda([this, From, to, amount, mint, existingAccount](FJsonObject& data)
	// 		{
	// 			const FString blockHash = FRequestUtils::ParseBlockHashResponse(data);
	//
	// 			const TArray<uint8> Transaction = FTransactionUtils::TransferTokenTransaction(
	// 				From, to, Account, amount, mint, blockHash, existingAccount);
	//
	// 			const auto sendTransaction = FRequestUtils::SendTransaction(FBase64::Encode(Transaction));
	// 			sendTransaction->Callback.BindLambda([this](FJsonObject& data)
	// 			{
	// 				const FString TransactionID = FRequestUtils::ParseTransactionResponse(data);
	// 				FRequestUtils::DisplayInfo(FString::Printf(TEXT("Transaction Id: %s"), *TransactionID));
	// 			});
	// 			FRequestManager::SendRequest(sendTransaction);
	// 		});
	// 		FRequestManager::SendRequest(BlockhashRequest);
	// 	}
	// });
	// FRequestManager::SendRequest(accountRequest);
}
