#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Containers/UnrealString.h"
#include "Containers/Array.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

class FLinstenerTask : public FNonAbandonableTask
{
public:
	FSocket* ListenSocket;

	FLinstenerTask(FSocket* InSocket)
		: ListenSocket(InSocket)
	{}

	FORCEINLINE TStatId GetStatId() const 
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLinstenerTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
	void HandleData(const TArray<uint8>& Data);
};
