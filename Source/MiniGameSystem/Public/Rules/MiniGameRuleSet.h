#pragma once

#include "CoreMinimal.h"
#include "Core/MiniGameTypes.h"
#include "UObject/Object.h"
#include "MiniGameRuleSet.generated.h"

class AMiniGameGameStateBase;
class APlayerState;

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class MINIGAMESYSTEM_API UMiniGameRuleSet : public UObject
{
	GENERATED_BODY()

public:
	// 룰셋 초기화
	UFUNCTION(BlueprintCallable, Category="MiniGame Rules")
	virtual void InitializeRules(const FMiniGameSetup& InSetup);

	// 종료 조건 판단
	UFUNCTION(BlueprintCallable, Category="MiniGame Rules")
	virtual bool ShouldFinishGame(const AMiniGameGameStateBase* GameState) const;

	// 점수 처리
	UFUNCTION(BlueprintCallable, Category="MiniGame Rules")
	virtual void AddScore(AMiniGameGameStateBase* GameState, APlayerState* PlayerState, int32 DeltaScore);

	// 결과 정리
	UFUNCTION(BlueprintCallable, Category="MiniGame Rules")
	virtual void ResolveRanking(AMiniGameGameStateBase* GameState) const;

	UFUNCTION(BlueprintCallable, Category="MiniGame Rules")
	virtual FMiniGameResult BuildMiniGameResult(const AMiniGameGameStateBase* GameState) const;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MiniGame Rules")
	FMiniGameSetup CachedSetup;
};
