#include "Rules/MiniGameRuleSet.h"

#include "GameFramework/PlayerState.h"
#include "Runtime/MiniGameGameStateBase.h"

void UMiniGameRuleSet::InitializeRules(const FMiniGameSetup& InSetup)
{
	// RuleSet이 공통으로 참조할 미니게임 설정을 캐싱
	CachedSetup = InSetup;
}

bool UMiniGameRuleSet::ShouldFinishGame(const AMiniGameGameStateBase* GameState) const
{
	// 기본적으로는 자동으로 게임을 종료하는 조건이 없다고 가정하는게 맞기 때문에 false
	return false;
}

void UMiniGameRuleSet::AddScore(AMiniGameGameStateBase* GameState, APlayerState* PlayerState, int32 DeltaScore)
{
	if (GameState == nullptr || PlayerState == nullptr)
	{
		return;
	}

	// 기본 구현은 단순 더하기로 점수를 기록하고, 개별 미니게임에서 오버라이드 가능
	GameState->AddScore(PlayerState, DeltaScore);
}

void UMiniGameRuleSet::ResolveRanking(AMiniGameGameStateBase* GameState) const
{
	if (GameState == nullptr)
	{
		return;
	}

	TArray<FMiniGameScoreEntry> SortedScoreBoard = GameState->GetScoreBoard();
	// 점수 내림차순으로 정렬해 최종 순위를 확정
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
	
	// GameState에 모인 점수판을 결과 구조체로 옮겨두고 이를 사용
	Result.ScoreBoard = GameState->GetScoreBoard();
	for (const FMiniGameScoreEntry& Entry : Result.ScoreBoard)
	{
		Result.Ranking.Add(Entry.PlayerState);
	}

	return Result;
}
