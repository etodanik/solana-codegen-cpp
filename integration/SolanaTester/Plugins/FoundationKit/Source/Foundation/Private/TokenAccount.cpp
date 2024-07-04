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

#include "TokenAccount.h"

#include "JsonObjectConverter.h"
#include "WalletAccount.h"
#include "Network/RequestManager.h"
#include "Network/RequestUtils.h"

void UTokenAccount::Update()
{
	const auto AccountRequest = FRequestUtils::RequestTokenAccount(AccountData.Pubkey, AccountData.Mint);
	AccountRequest->Callback.BindLambda([this](FJsonObject& Data)
	{
		if (const TSharedPtr<FJsonObject> Result = Data.GetObjectField("result"))
		{
			FTokenBalanceDataJson JsonData;
			FJsonObjectConverter::JsonObjectToUStruct(Result.ToSharedRef(), &JsonData);

			const FTokenInfoJson Info = JsonData.account.data.parsed.info;
			AccountData.Balance = Info.tokenAmount.uiAmount;
			OnBalanceUpdated.Broadcast(this, AccountData.Balance);
		}
	});
}

void UTokenAccount::Send(FString RecipientPublicKey, float Amount)
{
	UWalletAccount* WalletAccountOwner = Cast<UWalletAccount>(GetOuter());
	WalletAccountOwner->SendToken(this, RecipientPublicKey, Amount);
}
