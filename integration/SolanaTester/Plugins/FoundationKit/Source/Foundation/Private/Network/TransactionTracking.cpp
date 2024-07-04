#include "Network/TransactionTracking.h"
#include "Network/UGI_WebSocketManager.h"
#include "Network/SubscriptionUtils.h"

void FTransactionTracker::Sub2Transaction(const FString TransactionSignature, UGI_WebSocketManager*& SocketManager)
{
	FSubscriptionData* SubRequest = FSubscriptionUtils::SignatureSubscribe(TransactionSignature);
	SocketManager->Subscribe(SubRequest);
}

int FTransactionTracker::GetTransactionErr(const int TransactionID, UGI_WebSocketManager*& SocketManager)
{
	FSubscriptionData* Transaction = SocketManager->ActiveSubscriptions[TransactionID];
	TSharedPtr<FJsonObject> Result = FSubscriptionUtils::GetSignatureSubInfo(Transaction);
	if(Result.IsValid())
	{
		// TODO: Find out what the people who wrote this were smoking
		// if(Result->GetObjectField("value")->GetIntegerField("err") != nullptr){
		if(Result->GetObjectField("value")->GetIntegerField("err")){
			TSharedPtr<FJsonObject> Err = Result->GetObjectField("value")->GetObjectField("err");
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Transaction Error");
			return -1;
		}else
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Turquoise, "Transaction Completed without errors");
			return 0;
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Not possible to parse error!");
	return -1;
}

int FTransactionTracker::GetTransactionSlot(const int TransactionID, UGI_WebSocketManager*& SocketManager)
{
	FSubscriptionData* Transaction = SocketManager->ActiveSubscriptions[TransactionID];
	TSharedPtr<FJsonObject> Result = FSubscriptionUtils::GetSignatureSubInfo(Transaction);
	return Result->GetObjectField("context")->GetIntegerField("slot");
}

