#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "IWebSocket.h"
#include "UGI_WebSocketManager.generated.h"

struct FOUNDATION_API FSubscriptionData
{
	FSubscriptionData() {}
	FSubscriptionData( uint32 id ) { Id = id; }

	uint32 Id;
	uint32 SubscriptionNumber = 0;
	FString Body;
	FString UnsubMsg;
	TSharedPtr<FJsonObject> Response;
};

UCLASS()
class  FOUNDATION_API UGI_WebSocketManager:  public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void OnStart() override;
	TSharedPtr<IWebSocket> WebSocket;
	DECLARE_EVENT(UGI_WebSocketManager, FSocketConnected);

	static int64 GetNextSubID();
	static int64 GetLastSubID();
	inline  static TMap<int, FSubscriptionData*> ActiveSubscriptions;

	void Subscribe(FSubscriptionData* SubData);
	void Unsubscribe(int subID);
	FSubscriptionData* GetSubData(int SubID);
	void InitializeHeartbeat();
	UFUNCTION()
	void HeartbeatHelper();

private:
	inline static FSocketConnected OnConnected;
    static void OnResponse(const FString &Response);
	static void ParseNotification(const FString &Response);
	static void ParseSubConfirmation(const FString &Response);
    static void OnConnected_Helper();
};
