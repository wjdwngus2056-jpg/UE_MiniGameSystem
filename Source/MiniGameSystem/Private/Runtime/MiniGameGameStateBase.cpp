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

	// 진행 상태와 점수판은 모든 클라이언트가 동일하게 봐야하기 때문에 복제
	DOREPLIFETIME(AMiniGameGameStateBase, CurrentState);
	DOREPLIFETIME(AMiniGameGameStateBase, RemainingTimeSeconds);
	DOREPLIFETIME(AMiniGameGameStateBase, Participants);
	DOREPLIFETIME(AMiniGameGameStateBase, ScoreBoard);
}

void AMiniGameGameStateBase::SetMiniGameState(FGameplayTag NewState)
{
	// 현재 진행 단계를 서버에서 갱신
	CurrentState = NewState;
}

void AMiniGameGameStateBase::SetRemainingTimeSeconds(float NewRemainingTimeSeconds)
{
	// UI에서 바로 쓸 수 있도록 남은 시간을 저장
	RemainingTimeSeconds = NewRemainingTimeSeconds;
}

void AMiniGameGameStateBase::SetParticipants(const TArray<FMiniGameParticipantInfo>& InParticipants)
{
	// 현재 미니게임에 참여 중인 플레이어 목록을 교체
	Participants = InParticipants;
}

void AMiniGameGameStateBase::SetScoreBoard(const TArray<FMiniGameScoreEntry>& InScoreBoard)
{
	// 룰셋에서 정리한 점수판을 통째로 반영할 때 사용
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
		// 첫 득점한 플레이어는 점수판 엔트리를 새로 만들어서 추가
		ScoreBoard.Add(NewEntry);
		return;
	}

	ExistingEntry->PlayerName = PlayerState->GetPlayerName();
	// 이미 점수를 획득했던 플레이어는 점수를 누적하는 방식
	ExistingEntry->Score += DeltaScore;
}

int32 AMiniGameGameStateBase::GetScore(APlayerState* PlayerState) const
{
	if (PlayerState == nullptr)
	{
		return 0;
	}
	// 점수판에서 해당 플레이어의 현재 점수를 조회
	const FMiniGameScoreEntry* ExistingEntry = ScoreBoard.FindByPredicate([PlayerState](const FMiniGameScoreEntry& Entry)
	{
		return Entry.PlayerState == PlayerState;
	});

	return ExistingEntry != nullptr ? ExistingEntry->Score : 0;
}
