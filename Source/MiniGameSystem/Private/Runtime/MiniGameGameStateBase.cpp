#include "Runtime/MiniGameGameStateBase.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

AMiniGameGameStateBase::AMiniGameGameStateBase()
{
	bReplicates = true;
}

void AMiniGameGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMiniGameGameStateBase, CurrentState);
	DOREPLIFETIME(AMiniGameGameStateBase, RemainingTimeSeconds);
	DOREPLIFETIME(AMiniGameGameStateBase, Participants);
	DOREPLIFETIME(AMiniGameGameStateBase, ScoreBoard);
}

void AMiniGameGameStateBase::SetMiniGameState(EMiniGameState NewState)
{
	CurrentState = NewState;
}

void AMiniGameGameStateBase::SetRemainingTimeSeconds(float NewRemainingTimeSeconds)
{
	RemainingTimeSeconds = NewRemainingTimeSeconds;
}

void AMiniGameGameStateBase::SetParticipants(const TArray<FMiniGameParticipantInfo>& InParticipants)
{
	Participants = InParticipants;
}

void AMiniGameGameStateBase::SetScoreBoard(const TArray<FMiniGameScoreEntry>& InScoreBoard)
{
	ScoreBoard = InScoreBoard;
}

void AMiniGameGameStateBase::AddScore(APlayerState* PlayerState, int32 DeltaScore)
{
	if (PlayerState == nullptr)
	{
		return;
	}

	FMiniGameScoreEntry* ExistingEntry = ScoreBoard.FindByPredicate([PlayerState](const FMiniGameScoreEntry& Entry)
	{
		return Entry.PlayerState == PlayerState;
	});

	if (ExistingEntry == nullptr)
	{
		FMiniGameScoreEntry NewEntry;
		NewEntry.PlayerState = PlayerState;
		NewEntry.PlayerName = PlayerState->GetPlayerName();
		NewEntry.Score = DeltaScore;
		ScoreBoard.Add(NewEntry);
		return;
	}

	ExistingEntry->PlayerName = PlayerState->GetPlayerName();
	ExistingEntry->Score += DeltaScore;
}

int32 AMiniGameGameStateBase::GetScore(APlayerState* PlayerState) const
{
	if (PlayerState == nullptr)
	{
		return 0;
	}

	const FMiniGameScoreEntry* ExistingEntry = ScoreBoard.FindByPredicate([PlayerState](const FMiniGameScoreEntry& Entry)
	{
		return Entry.PlayerState == PlayerState;
	});

	return ExistingEntry != nullptr ? ExistingEntry->Score : 0;
}
