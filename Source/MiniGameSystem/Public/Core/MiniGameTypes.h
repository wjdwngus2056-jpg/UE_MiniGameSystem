#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MiniGameTypes.generated.h"

class APlayerState;
class UWorld;

UENUM(BlueprintType)
enum class EMiniGameState : uint8
{
	Idle,
	Preparing,
	Traveling,
	Playing,
	Finishing,
	Returning,
	Completed
};

UENUM(BlueprintType)
enum class EMiniGameGenre : uint8
{
	None,
	Competition,
	Survival,
	Race
};

UENUM(BlueprintType)
enum class EMiniGameFinishReason : uint8
{
	TimeOver,
	Completed
};

USTRUCT(BlueprintType)
struct MINIGAMESYSTEM_API FMiniGameParticipantInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Id = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bConnected = true;
};

USTRUCT(BlueprintType)
struct MINIGAMESYSTEM_API FMiniGameScoreEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Score = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Rank = 0;
};

USTRUCT(BlueprintType)
struct MINIGAMESYSTEM_API FMiniGameSetup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName MiniGameId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EMiniGameGenre Genre = EMiniGameGenre::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MinPlayers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxPlayers = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TimeLimitSeconds = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer SessionTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> MiniGameMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ReturnContextId = NAME_None;
};

USTRUCT(BlueprintType)
struct MINIGAMESYSTEM_API FMiniGameResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName MiniGameId = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	EMiniGameFinishReason FinishReason = EMiniGameFinishReason::TimeOver;

	UPROPERTY(BlueprintReadOnly)
	TArray<FMiniGameScoreEntry> ScoreBoard;

	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<APlayerState>> Ranking;
};
