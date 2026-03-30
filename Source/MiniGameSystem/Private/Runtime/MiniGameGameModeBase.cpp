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

	if (!HasAuthority() || GetGameInstance() == nullptr)
	{
		return;
	}

	UMiniGameManagerSubsystem* MiniGameManager = GetGameInstance()->GetSubsystem<UMiniGameManagerSubsystem>();
	if (MiniGameManager == nullptr || !MiniGameManager->HasActiveMiniGame() || !MiniGameManager->IsActiveMiniGameWorld(GetWorld()))
	{
		return;
	}

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
		const float RemainingTimeSeconds = ActiveSetup.TimeLimitSeconds > 0.f
			? FMath::Max(0.f, ActiveSetup.TimeLimitSeconds - ElapsedTimeSeconds)
			: 0.f;
		MiniGameState->SetRemainingTimeSeconds(RemainingTimeSeconds);
	}

	if (RuleSet != nullptr && RuleSet->ShouldFinishGame(GetMiniGameGameState()))
	{
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
		MiniGameState->SetParticipants(ActiveParticipants);
		MiniGameState->SetRemainingTimeSeconds(ActiveSetup.TimeLimitSeconds);
		MiniGameState->SetMiniGameState(FMiniGameNativeTags::Get().State_Preparing);
		MiniGameState->SetScoreBoard(TArray<FMiniGameScoreEntry>());
	}
}

void AMiniGameGameModeBase::StartMiniGame()
{
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
	FinishMiniGame(FMiniGameNativeTags::Get().FinishReason_Completed);
}

void AMiniGameGameModeBase::StopMiniGame(FGameplayTag Reason)
{
	FinishMiniGame(Reason);
}

FMiniGameResult AMiniGameGameModeBase::BuildMiniGameResult() const
{
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
		RuleSet->InitializeRules(ActiveSetup);
	}
}
