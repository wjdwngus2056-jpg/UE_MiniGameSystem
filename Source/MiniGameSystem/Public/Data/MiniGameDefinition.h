#pragma once

#include "CoreMinimal.h"
#include "Core/MiniGameTypes.h"
#include "Engine/DataAsset.h"
#include "Rules/MiniGameRuleSet.h"
#include "Runtime/MiniGameGameModeBase.h"
#include "MiniGameDefinition.generated.h"
class UTexture2D;

UCLASS(BlueprintType)
class MINIGAMESYSTEM_API UMiniGameDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	FName MiniGameId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame", meta=(Categories="MiniGame.Genre"))
	FGameplayTag Genre = FMiniGameNativeTags::Get().Genre_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	int32 MinPlayers = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	int32 MaxPlayers = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	float DefaultTimeLimitSeconds = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	FGameplayTagContainer MiniGameTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	TSoftObjectPtr<UWorld> MiniGameMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	TSubclassOf<AMiniGameGameModeBase> GameModeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame")
	TSubclassOf<UMiniGameRuleSet> RuleSetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MiniGame|UI")
	TObjectPtr<UTexture2D> Thumbnail = nullptr;
};
