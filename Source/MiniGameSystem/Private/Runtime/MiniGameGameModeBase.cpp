#include "Runtime/MiniGameGameModeBase.h"

#include "Data/MiniGameDefinition.h"
#include "Rules/MiniGameRuleSet.h"
#include "Runtime/MiniGameGameStateBase.h"
#include "Runtime/MiniGameManagerSubsystem.h"

AMiniGameGameModeBase::AMiniGameGameModeBase()
{
	GameStateClass = AMiniGameGameStateBase::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void AMiniGameGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// GameMode는 서버에만 존재하므로 서버가 아닌 경우 바로 종료
	if (!HasAuthority() || GetGameInstance() == nullptr)
	{
		return;
	}

	UMiniGameManagerSubsystem* MiniGameManager = GetGameInstance()->GetSubsystem<UMiniGameManagerSubsystem>();
	if (MiniGameManager == nullptr || !MiniGameManager->HasActiveMiniGame() || !MiniGameManager->IsActiveMiniGameWorld(GetWorld()))
	{
		return;
	}

	// 서버에서만 저장된 미니게임 세팅을 가져오고, 실제 미니게임 플레이를 시작
	InitializeMiniGame(MiniGameManager->GetActiveSetup(), MiniGameManager->GetActiveParticipants());
	InitializeRuleSet();
	StartMiniGame();
	MiniGameManager->NotifyMiniGameStarted();
}

void AMiniGameGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FMiniGameNativeTags& MiniGameTags = FMiniGameNativeTags::Get();

	if (!HasAuthority() || !bMiniGameStarted || bMiniGameFinished)
	{
		return;
	}

	ElapsedTimeSeconds += DeltaSeconds;

	if (AMiniGameGameStateBase* MiniGameState = GetMiniGameGameState())
	{
		// 남은 시간은 서버에서 계산하고 GameState 복제로 모든 클라이언트와 동기화
		const float RemainingTimeSeconds = ActiveSetup.TimeLimitSeconds > 0.f
			? FMath::Max(0.f, ActiveSetup.TimeLimitSeconds - ElapsedTimeSeconds)
			: 0.f;
		MiniGameState->SetRemainingTimeSeconds(RemainingTimeSeconds);
	}

	if (RuleSet != nullptr && RuleSet->ShouldFinishGame(GetMiniGameGameState()))
	{
		// 개별 룰셋이 종료 조건을 만족했다고 판단하면 즉시 종료
		FinishMiniGame(MiniGameTags.FinishReason_Completed);
		return;
	}

	if (ActiveSetup.TimeLimitSeconds > 0.f && ElapsedTimeSeconds >= ActiveSetup.TimeLimitSeconds)
	{
		FinishMiniGame(MiniGameTags.FinishReason_TimeOver);
	}
}

void AMiniGameGameModeBase::InitializeMiniGame(const FMiniGameSetup& InSetup, const TArray<FMiniGameParticipantInfo>& InParticipants)
{
	ActiveSetup = InSetup;
	ActiveParticipants = InParticipants;
	bMiniGameStarted = false;
	bMiniGameFinished = false;
	ElapsedTimeSeconds = 0.f;

	if (AMiniGameGameStateBase* MiniGameState = GetMiniGameGameState())
	{
		// 클라이언트가 확인할 초기 상태는 GameState에 복제 가능한 형태로 저장
		MiniGameState->SetParticipants(ActiveParticipants);
		MiniGameState->SetRemainingTimeSeconds(ActiveSetup.TimeLimitSeconds);
		MiniGameState->SetMiniGameState(FMiniGameNativeTags::Get().State_Preparing);
		MiniGameState->SetScoreBoard(TArray<FMiniGameScoreEntry>());
	}
}

void AMiniGameGameModeBase::StartMiniGame()
{
	// 실제 플레이 시작 시점을 기록하고 상태를 Playing으로 바꾸는 부분
	bMiniGameStarted = true;
	ElapsedTimeSeconds = 0.f;

	if (AMiniGameGameStateBase* MiniGameState = GetMiniGameGameState())
	{
		MiniGameState->SetMiniGameState(FMiniGameNativeTags::Get().State_Playing);
		MiniGameState->SetRemainingTimeSeconds(ActiveSetup.TimeLimitSeconds);
	}
}

void AMiniGameGameModeBase::RequestFinishMiniGame()
{
	// 기본적으로 사용하는 종료 요청
	FinishMiniGame(FMiniGameNativeTags::Get().FinishReason_Completed);
}

void AMiniGameGameModeBase::StopMiniGame(FGameplayTag Reason)
{
	// 외부에서 게임 종료의 원인을 태그로 받아 미니게임을 강제로 종료
	FinishMiniGame(Reason);
}

FMiniGameResult AMiniGameGameModeBase::BuildMiniGameResult() const
{
	// 룰셋이 있으면 룰셋 결과를 우선 사용하고, 없으면 기본 결과를 생성하는 방식
	FMiniGameResult Result = RuleSet != nullptr
		? RuleSet->BuildMiniGameResult(GetMiniGameGameState())
		: FMiniGameResult();

	Result.MiniGameId = ActiveSetup.MiniGameId;
	if (const AMiniGameGameStateBase* MiniGameState = GetMiniGameGameState())
	{
		Result.ScoreBoard = MiniGameState->GetScoreBoard();
		if (Result.Ranking.Num() == 0)
		{
			for (const FMiniGameScoreEntry& Entry : Result.ScoreBoard)
			{
				Result.Ranking.Add(Entry.PlayerState);
			}
		}
	}

	return Result;
}

void AMiniGameGameModeBase::AddScore(APlayerState* PlayerState, int32 DeltaScore)
{
	AMiniGameGameStateBase* MiniGameState = GetMiniGameGameState();
	if (MiniGameState == nullptr || PlayerState == nullptr)
	{
		return;
	}

	if (RuleSet != nullptr)
	{
		// 점수 계산 규칙이 있으면 룰셋에서 처리
		RuleSet->AddScore(MiniGameState, PlayerState, DeltaScore);
	}
	else
	{
		MiniGameState->AddScore(PlayerState, DeltaScore);
	}
}

void AMiniGameGameModeBase::FinishMiniGame(FGameplayTag Reason)
{
	if (bMiniGameFinished)
	{
		return;
	}

	bMiniGameFinished = true;

	if (AMiniGameGameStateBase* MiniGameState = GetMiniGameGameState())
	{
		// 종료 직전 점수판을 정리하고 최종 상태를 Completed로 확정한다.
		MiniGameState->SetMiniGameState(FMiniGameNativeTags::Get().State_Finishing);
		if (RuleSet != nullptr)
		{
			RuleSet->ResolveRanking(MiniGameState);
		}
		MiniGameState->SetRemainingTimeSeconds(0.f);
		MiniGameState->SetMiniGameState(FMiniGameNativeTags::Get().State_Completed);
	}

	FMiniGameResult Result = BuildMiniGameResult();
	Result.FinishReason = Reason;

	if (GetGameInstance() != nullptr)
	{
		if (UMiniGameManagerSubsystem* MiniGameManager = GetGameInstance()->GetSubsystem<UMiniGameManagerSubsystem>())
		{
			// 최종 결과 저장과 맵 복귀는 Subsystem이 일괄 관리한다.
			MiniGameManager->CommitMiniGameResult(Result);
		}
	}
}

AMiniGameGameStateBase* AMiniGameGameModeBase::GetMiniGameGameState() const
{
	return GetGameState<AMiniGameGameStateBase>();
}

void AMiniGameGameModeBase::InitializeRuleSet()
{
	RuleSet = nullptr;

	if (GetGameInstance() == nullptr)
	{
		return;
	}

	UMiniGameManagerSubsystem* MiniGameManager = GetGameInstance()->GetSubsystem<UMiniGameManagerSubsystem>();
	UMiniGameDefinition* Definition = MiniGameManager != nullptr ? MiniGameManager->GetActiveDefinition() : nullptr;
	if (Definition == nullptr || Definition->RuleSetClass == nullptr)
	{
		return;
	}

	RuleSet = NewObject<UMiniGameRuleSet>(this, Definition->RuleSetClass);
	if (RuleSet != nullptr)
	{
		// 룰셋은 미니게임별 종료 조건과 점수 처리 정책을 캡슐화한다.
		RuleSet->InitializeRules(ActiveSetup);
	}
}
