#pragma once

#include "CoreMinimal.h"
#include "Core/MiniGameTypes.h"
#include "UObject/Interface.h"
#include "MiniGameInstanceInterface.generated.h"

UINTERFACE(BlueprintType)
class MINIGAMESYSTEM_API UMiniGameInstanceInterface : public UInterface
{
	GENERATED_BODY()
};

class MINIGAMESYSTEM_API IMiniGameInstanceInterface
{
	GENERATED_BODY()

public:
	virtual void InitializeMiniGame(const FMiniGameSetup& InSetup, const TArray<FMiniGameParticipantInfo>& InParticipants) = 0;
	virtual void StartMiniGame() = 0;
	virtual void RequestFinishMiniGame() = 0;
	virtual void StopMiniGame(FGameplayTag Reason) = 0;
	virtual FMiniGameResult BuildMiniGameResult() const = 0;
};
