#pragma once

#include "CoreMinimal.h"
#include "Core/MiniGameTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MiniGameManagerSubsystem.generated.h"

class UMiniGameDefinition;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGameStateChanged, EMiniGameState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGamePreparing, const FMiniGameSetup&, Setup);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMiniGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGameFinished, const FMiniGameResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGameResultCommitted, const FMiniGameResult&, Result);

UCLASS()
class MINIGAMESYSTEM_API UMiniGameManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	bool RequestStartMiniGame(FName MiniGameId, const TArray<APlayerState*>& Players);

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	bool RequestFinishMiniGame();

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void StopActiveMiniGame(EMiniGameFinishReason Reason);

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void CommitMiniGameResult(const FMiniGameResult& Result);

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void NotifyMiniGameStarted();

	UFUNCTION(BlueprintPure, Category="MiniGame")
	EMiniGameState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	const FMiniGameSetup& GetActiveSetup() const { return ActiveSetup; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	const TArray<FMiniGameParticipantInfo>& GetActiveParticipants() const { return ActiveParticipants; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	UMiniGameDefinition* GetActiveDefinition() const { return ActiveDefinition; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	bool HasActiveMiniGame() const { return ActiveDefinition != nullptr; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	const FMiniGameResult& GetLastCommittedResult() const { return LastCommittedResult; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	bool HasLastCommittedResult() const { return bHasLastCommittedResult; }

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void ClearLastCommittedResult();

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	UMiniGameDefinition* FindMiniGameDefinition(FName MiniGameId);

	bool IsActiveMiniGameWorld(const UWorld* World) const;

public:
	UPROPERTY(BlueprintAssignable, Category="MiniGame")
	FOnMiniGamePreparing OnMiniGamePreparing;
	UPROPERTY(BlueprintAssignable, Category="MiniGame")
	FOnMiniGameStarted OnMiniGameStarted;
	UPROPERTY(BlueprintAssignable, Category="MiniGame")
	FOnMiniGameStateChanged OnMiniGameStateChanged;
	UPROPERTY(BlueprintAssignable, Category="MiniGame")
	FOnMiniGameFinished OnMiniGameFinished;
	UPROPERTY(BlueprintAssignable, Category="MiniGame")
	FOnMiniGameResultCommitted OnMiniGameResultCommitted;

protected:
	UPROPERTY(VisibleInstanceOnly, Category="MiniGame")
	EMiniGameState CurrentState = EMiniGameState::Idle;

	UPROPERTY(VisibleInstanceOnly, Category="MiniGame")
	FMiniGameSetup ActiveSetup;

	UPROPERTY(VisibleInstanceOnly, Category="MiniGame")
	TArray<FMiniGameParticipantInfo> ActiveParticipants;

	UPROPERTY(VisibleInstanceOnly, Category="MiniGame")
	TObjectPtr<UMiniGameDefinition> ActiveDefinition = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category="MiniGame")
	FMiniGameResult LastCommittedResult;

	UPROPERTY(VisibleInstanceOnly, Category="MiniGame")
	bool bHasLastCommittedResult = false;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMiniGameDefinition>> CachedDefinitions;

	UPROPERTY()
	FString ReturnMapPackageName;

	void SetCurrentState(EMiniGameState NewState);
	bool BuildSetupFromDefinition(const UMiniGameDefinition* Definition, const TArray<APlayerState*>& Players);
	void ClearActiveSession();
	bool TravelToMap(const TSoftObjectPtr<UWorld>& MapAsset);
	void CacheAvailableDefinitions();
};
