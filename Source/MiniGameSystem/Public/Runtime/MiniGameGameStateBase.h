#pragma once

#include "CoreMinimal.h"
#include "Core/MiniGameTypes.h"
#include "GameFramework/GameStateBase.h"
#include "MiniGameGameStateBase.generated.h"

UCLASS(Blueprintable)
class MINIGAMESYSTEM_API AMiniGameGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	AMiniGameGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintPure, Category="MiniGame")
	FGameplayTag GetMiniGameState() const { return CurrentState; }
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void SetMiniGameState(FGameplayTag NewState);
	UFUNCTION(BlueprintPure, Category="MiniGame")
	bool IsMiniGameState(FGameplayTag StateTag) const { return CurrentState == StateTag; }
	UFUNCTION(BlueprintPure, Category="MiniGame")
	float GetRemainingTimeSeconds() const { return RemainingTimeSeconds; }
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void SetRemainingTimeSeconds(float NewRemainingTimeSeconds);
	UFUNCTION(BlueprintPure, Category="MiniGame")
	const TArray<FMiniGameParticipantInfo>& GetParticipants() const { return Participants; }
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void SetParticipants(const TArray<FMiniGameParticipantInfo>& InParticipants);
	UFUNCTION(BlueprintPure, Category="MiniGame")
	const TArray<FMiniGameScoreEntry>& GetScoreBoard() const { return ScoreBoard; }
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void SetScoreBoard(const TArray<FMiniGameScoreEntry>& InScoreBoard);
	UFUNCTION(BlueprintCallable, Category="MiniGame")
	void AddScore(APlayerState* PlayerState, int32 DeltaScore);
	UFUNCTION(BlueprintPure, Category="MiniGame")
	int32 GetScore(APlayerState* PlayerState) const;

protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="MiniGame", meta=(Categories="MiniGame.State"))
	FGameplayTag CurrentState = FMiniGameNativeTags::Get().State_Idle;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="MiniGame")
	float RemainingTimeSeconds = 0.f;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="MiniGame")
	TArray<FMiniGameParticipantInfo> Participants;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="MiniGame")
	TArray<FMiniGameScoreEntry> ScoreBoard;
};
