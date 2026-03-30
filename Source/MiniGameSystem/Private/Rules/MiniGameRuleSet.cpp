#include "Rules/MiniGameRuleSet.h"

#include "GameFramework/PlayerState.h"
#include "Runtime/MiniGameGameStateBase.h"

void UMiniGameRuleSet::InitializeRules(const FMiniGameSetup& InSetup)
{
	CachedSetup = InSetup;
}

bool UMiniGameRuleSet::ShouldFinishGame(const AMiniGameGameStateBase* GameState) const
{
	return false;
}

void UMiniGameRuleSet::AddScore(AMiniGameGameStateBase* GameState, APlayerState* PlayerState, int32 DeltaScore)
{
	if (GameState == nullptr || PlayerState == nullptr)
	{
		return;
	}

	GameState->AddScore(PlayerState, DeltaScore);
}

void UMiniGameRuleSet::ResolveRanking(AMiniGameGameStateBase* GameState) const
{
	if (GameState == nullptr)
	{
		return;
	}

	TArray<FMiniGameScoreEntry> SortedScoreBoard = GameState->GetScoreBoard();
	SortedScoreBoard.Sort([](const FMiniGameScoreEntry& Left, const FMiniGameScoreEntry& Right)
	{
		return Left.Score > Right.Score;
	});

	for (int32 Index = 0; Index < SortedScoreBoard.Num(); ++Index)
	{
		SortedScoreBoard[Index].Rank = Index + 1;
	}

	GameState->SetScoreBoard(SortedScoreBoard);
}

FMiniGameResult UMiniGameRuleSet::BuildMiniGameResult(const AMiniGameGameStateBase* GameState) const
{
	FMiniGameResult Result;
	Result.MiniGameId = CachedSetup.MiniGameId;

	if (GameState == nullptr)
	{
		return Result;
	}

	Result.ScoreBoard = GameState->GetScoreBoard();
	for (const FMiniGameScoreEntry& Entry : Result.ScoreBoard)
	{
		Result.Ranking.Add(Entry.PlayerState);
	}

	return Result;
}
