#pragma once

#include "CoreMinimal.h"
#include "Core/MiniGameTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MiniGameManagerSubsystem.generated.h"

class UMiniGameDefinition;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGameStateChanged, FGameplayTag, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGamePreparing, const FMiniGameSetup&, Setup);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMiniGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGameFinished, const FMiniGameResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniGameResultCommitted, const FMiniGameResult&, Result);

UCLASS()
class MINIGAMESYSTEM_API UMiniGameManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 서브시스템 생명주기 관련
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 미니게임 시작/종료 제어
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	bool RequestStartMiniGame(FName MiniGameId, const TArray<APlayerState*>& Players);

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	bool RequestFinishMiniGame();

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void StopActiveMiniGame(FGameplayTag Reason);

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void CommitMiniGameResult(const FMiniGameResult& Result);

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void NotifyMiniGameStarted();

	// 현재 실행 상태 조회
	UFUNCTION(BlueprintPure, Category="MiniGame")
	FGameplayTag GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	const FMiniGameSetup& GetActiveSetup() const { return ActiveSetup; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	const TArray<FMiniGameParticipantInfo>& GetActiveParticipants() const { return ActiveParticipants; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	UMiniGameDefinition* GetActiveDefinition() const { return ActiveDefinition; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	bool HasActiveMiniGame() const { return ActiveDefinition != nullptr; }

	// 마지막 결과 조회
	UFUNCTION(BlueprintPure, Category="MiniGame")
	const FMiniGameResult& GetLastCommittedResult() const { return LastCommittedResult; }

	UFUNCTION(BlueprintPure, Category="MiniGame")
	bool HasLastCommittedResult() const { return bHasLastCommittedResult; }

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void ClearLastCommittedResult();

	// 미니게임 정의 조회
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	UMiniGameDefinition* FindMiniGameDefinition(FName MiniGameId);

	// 현재 월드가 활성 미니게임 월드인지 판별
	bool IsActiveMiniGameWorld(const UWorld* World) const;

public:
	// 상태 변화 알림
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
	UPROPERTY(VisibleInstanceOnly, Category="MiniGame", meta=(Categories="MiniGame.State"))
	FGameplayTag CurrentState = FMiniGameNativeTags::Get().State_Idle;

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

	// 내부 상태 전환 및 세팅 구성
	void SetCurrentState(FGameplayTag NewState);
	bool BuildSetupFromDefinition(const UMiniGameDefinition* Definition, const TArray<APlayerState*>& Players);

	// 런타임 데이터 초기화와 맵 이동 처리
	void ClearActiveSession();
	bool TravelToMap(const TSoftObjectPtr<UWorld>& MapAsset);

	// 데이터 에셋 검색 및 캐시
	void CacheAvailableDefinitions();
};
