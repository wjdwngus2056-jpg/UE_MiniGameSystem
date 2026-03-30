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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void InitializeMiniGame(const FMiniGameSetup& InSetup, const TArray<FMiniGameParticipantInfo>& InParticipants) override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void StartMiniGame() override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void RequestFinishMiniGame() override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual void StopMiniGame(EMiniGameFinishReason Reason) override;

	UFUNCTION(BlueprintCallable, Category="MiniGame")
	virtual FMiniGameResult BuildMiniGameResult() const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MiniGame")
	void AddScore(APlayerState* PlayerState, int32 DeltaScore);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MiniGame")
	void FinishMiniGame(EMiniGameFinishReason Reason);

protected:
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
