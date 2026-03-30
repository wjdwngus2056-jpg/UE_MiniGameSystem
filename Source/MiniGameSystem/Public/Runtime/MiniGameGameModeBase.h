#pragma once

#include "CoreMinimal.h"
#include "Core/MiniGameTypes.h"
#include "GameFramework/GameModeBase.h"
#include "Interface/MiniGameInstanceInterface.h"
#include "MiniGameGameModeBase.generated.h"

class AMiniGameGameStateBase;
class UMiniGameRuleSet;

UCLASS(Abstract, Blueprintable)
class MINIGAMESYSTEM_API AMiniGameGameModeBase : public AGameModeBase, public IMiniGameInstanceInterface
{
	GENERATED_BODY()

public:
	AMiniGameGameModeBase();

	// 서버 진입 및 진행 루프
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// IMiniGameInstanceInterface 구현
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void InitializeMiniGame(const FMiniGameSetup& InSetup, const TArray<FMiniGameParticipantInfo>& InParticipants) override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void StartMiniGame() override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void RequestFinishMiniGame() override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void StopMiniGame(FGameplayTag Reason) override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual FMiniGameResult BuildMiniGameResult() const override;

	// 서버 전용 점수/종료 처리
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MiniGame")
	void AddScore(APlayerState* PlayerState, int32 DeltaScore);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MiniGame")
	void FinishMiniGame(FGameplayTag Reason);

protected:
	// 현재 라운드 실행 정보
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MiniGame")
	FMiniGameSetup ActiveSetup;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MiniGame")
	TArray<FMiniGameParticipantInfo> ActiveParticipants;

	UPROPERTY(Transient, BlueprintReadOnly, Category="MiniGame")
	TObjectPtr<UMiniGameRuleSet> RuleSet = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MiniGame")
	bool bMiniGameStarted = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MiniGame")
	bool bMiniGameFinished = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MiniGame")
	float ElapsedTimeSeconds = 0.f;
	
	AMiniGameGameStateBase* GetMiniGameGameState() const;
	void InitializeRuleSet();
};
