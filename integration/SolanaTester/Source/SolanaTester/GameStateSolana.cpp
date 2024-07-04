// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStateSolana.h"

#include "SolanaWallet.h"
#include "SolanaWalletManager.h"
#include "WalletAccount.h"
#include "Crypto/Base58.h"
#include "Network/RequestManager.h"
#include "Network/RequestUtils.h"
#include "Solana/Transaction.h"
#include "Solana/PublicKey.h"
#include "Solana/Crypto/ProgramDerivedAccount.h"
#include "SolanaProgram/Instructions/Initialize.h"
#include "SolanaProgram/Instructions/MoveLeft.h"
#include "SolanaProgram/Instructions/MoveRight.h"

AGameStateSolana::AGameStateSolana()
{
}

UWalletAccount* AGameStateSolana::EnsureUnlockedWallet(const FWalletIsLockedDelegate& OnWalletLocked)
{
	USolanaWalletManager* SolanaWalletManager = GetGameInstance()->GetSubsystem<USolanaWalletManager>();

	if (SolanaWalletManager == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to get SolanaWalletManager"));
		return nullptr;
	}
	
	USolanaWallet* Wallet = SolanaWalletManager->GetAllRegisteredWallets().Array()[0].Value;

	if (Wallet == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to get Wallet"));
		return nullptr;
	}

	if (Wallet->IsWalletLocked())
	{
		UE_LOG(LogTemp, Warning, TEXT("Wallet is locked"));
		OnWalletLocked.ExecuteIfBound();
		return nullptr;
	}
	
	UWalletAccount* WalletAccount = Wallet->GetAccounts()[0];

	if (WalletAccount == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("No account found in wallet"));
	}

	return WalletAccount;
}

#include "Crypto/CryptoUtils.h"

FString ToSnakeCase(const FString& Input)
{
	FString Result;
	for (const TCHAR Character : Input)
	{
		if (FChar::IsUpper(Character))
		{
			if (!Result.IsEmpty())
			{
				Result.AppendChar('_');
			}
			Result.AppendChar(FChar::ToLower(Character));
		}
		else
		{
			Result.AppendChar(Character);
		}
	}
	return Result;
}

TArray<uint8> GetAnchorInstructionSighash(const FString& Namespace, const FString& IxName)
{
	const FString Name = ToSnakeCase(IxName);
	const FString Preimage = Namespace + TEXT(":") + Name;

	const FTCHARToUTF8 Utf8String(*Preimage);
	const uint8* PreimageBytes = reinterpret_cast<const uint8*>(Utf8String.Get());
	TArray<uint8> Hash = FCryptoUtils::SHA256_Digest(PreimageBytes, Utf8String.Length());

	TArray<uint8> Result;
	for (int32 Index = 0; Index < 8 && Index < Hash.Num(); ++Index)
	{
		Result.Add(Hash[Index]);
	}

	return Result;
}

void AGameStateSolana::InitializeGame(const FGameInitializationCompleteDelegate OnGameInitializationComplete, const FWalletIsLockedDelegate& OnWalletLocked)
{
	if (TSharedPtr<UWalletAccount> WalletAccount = MakeShareable(EnsureUnlockedWallet(OnWalletLocked)))
	{
		const TSharedPtr<FRequestData> BlockHashRequest = FRequestUtils::RequestBlockHash();
		BlockHashRequest->Callback.BindLambda(
			[this, BlockHashRequest, WalletAccount, OnGameInitializationComplete](FJsonObject& BlockHashData)
			{
				auto PublicKey = FBase58::DecodeBase58(WalletAccount->GetPublicKey());
				TTuple<FString, int> GameDataPDA = FProgramDerivedAccount::FindProgramAddress(
					{
						FProgramDerivedAccount::StringToByteArray("level1")
					},
					GTinyAdventureID.DecodeBase58()
				);
				GameDataAccount = GameDataPDA.Get<0>();
				
				FInitializeInstruction InitializeInstruction(GameDataAccount, WalletAccount->GetPublicKey(), FPublicKey("11111111111111111111111111111111"));
				const FString BlockHash = FRequestUtils::ParseBlockHashResponse(BlockHashData);
				FTransaction InitializeTx(BlockHash);
				InitializeTx.AddInstruction(InitializeInstruction);
				TArray<uint8> InitializeTxData = InitializeTx.Build(WalletAccount->AccountData);
				const auto SendTransaction = FRequestUtils::SendTransaction(FBase64::Encode(InitializeTxData));
				SendTransaction->Callback.BindLambda([this, BlockHashRequest, OnGameInitializationComplete](const FJsonObject& Data)
				{
					const FString TransactionId = FRequestUtils::ParseTransactionResponse(Data);
					UE_LOG(LogTemp, Log, TEXT("Transaction Id: %s"), *TransactionId);
					OnGameInitializationComplete.ExecuteIfBound(true);
				});
				
				SendTransaction->ErrorCallback.BindLambda([this, BlockHashRequest, OnGameInitializationComplete](FString& Error) {
					UE_LOG(LogTemp, Error, TEXT("Error: %s"), *Error);
					OnGameInitializationComplete.ExecuteIfBound(false);
				});
				
				FRequestManager::SendRequest(SendTransaction);
			});
		FRequestManager::SendRequest(BlockHashRequest);
	}
}

void AGameStateSolana::SendMoveLeftTransaction(const FMoveLeftCompleteDelegate OnMoveLeftComplete, const FWalletIsLockedDelegate& OnWalletLocked)
{
	if (TSharedPtr<UWalletAccount> WalletAccount = MakeShareable(EnsureUnlockedWallet(OnWalletLocked)))
	{
		const auto BlockHashRequest = FRequestUtils::RequestBlockHash();
		BlockHashRequest->Callback.BindLambda(
			[this, BlockHashRequest, WalletAccount, OnMoveLeftComplete](FJsonObject& Data)
			{
				FMoveLeftInstruction	MoveLeftInstruction(GameDataAccount);
				const FString BlockHash = FRequestUtils::ParseBlockHashResponse(Data);
				FTransaction MoveLeftTx(BlockHash);
				MoveLeftTx.AddInstruction(MoveLeftInstruction);
				const auto SendTransaction = FRequestUtils::SendTransaction(FBase64::Encode(MoveLeftTx.Build(WalletAccount->AccountData)));
	
				SendTransaction->Callback.BindLambda([this, BlockHashRequest, OnMoveLeftComplete](const FJsonObject& Data)
				{
					const FString TransactionId = FRequestUtils::ParseTransactionResponse(Data);
					UE_LOG(LogTemp, Log, TEXT("Transaction Id: %s"), *TransactionId);
					OnMoveLeftComplete.ExecuteIfBound(true);
				});
	
				SendTransaction->ErrorCallback.BindLambda([this, BlockHashRequest, OnMoveLeftComplete](FString& Error) {
					UE_LOG(LogTemp, Error, TEXT("Error: %s"), *Error);
					OnMoveLeftComplete.ExecuteIfBound(false);
				});
								
				FRequestManager::SendRequest(SendTransaction);
			});
		FRequestManager::SendRequest(BlockHashRequest);
	}
}

void AGameStateSolana::SendMoveRightTransaction(const FMoveRightCompleteDelegate OnMoveRightComplete, const FWalletIsLockedDelegate& OnWalletLocked)
{
	if (TSharedPtr<UWalletAccount> WalletAccount = MakeShareable(EnsureUnlockedWallet(OnWalletLocked)))
	{
		const auto BlockHashRequest = FRequestUtils::RequestBlockHash();
		BlockHashRequest->Callback.BindLambda(
			[this, BlockHashRequest, WalletAccount, OnMoveRightComplete](FJsonObject& Data)
			{
				FMoveRightInstruction	MoveRightInstruction(GameDataAccount);
				const FString BlockHash = FRequestUtils::ParseBlockHashResponse(Data);
				FTransaction MoveRightTx(BlockHash);
				MoveRightTx.AddInstruction(MoveRightInstruction);
				const auto SendTransaction = FRequestUtils::SendTransaction(FBase64::Encode(MoveRightTx.Build(WalletAccount->AccountData)));
				
				SendTransaction->Callback.BindLambda([this, BlockHashRequest, OnMoveRightComplete](const FJsonObject& Data)
				{
					const FString TransactionId = FRequestUtils::ParseTransactionResponse(Data);
					UE_LOG(LogTemp, Log, TEXT("Transaction Id: %s"), *TransactionId);
					OnMoveRightComplete.ExecuteIfBound(true);
				});
	
				SendTransaction->ErrorCallback.BindLambda([this, BlockHashRequest, OnMoveRightComplete](FString& Error) {
					UE_LOG(LogTemp, Error, TEXT("Error: %s"), *Error);
					OnMoveRightComplete.ExecuteIfBound(false);
				});
				
				FRequestManager::SendRequest(SendTransaction);
			});
		FRequestManager::SendRequest(BlockHashRequest);
	}
}
