#include "Runtime/MiniGameManagerSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/MiniGameDefinition.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Interface/MiniGameInstanceInterface.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

void UMiniGameManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CacheAvailableDefinitions();
	ClearLastCommittedResult();
	ClearActiveSession();
}

void UMiniGameManagerSubsystem::Deinitialize()
{
	CachedDefinitions.Reset();
	ClearLastCommittedResult();
	ClearActiveSession();

	Super::Deinitialize();
}

bool UMiniGameManagerSubsystem::RequestStartMiniGame(FName MiniGameId, const TArray<APlayerState*>& Players)
{
	if (CurrentState != EMiniGameState::Idle || MiniGameId.IsNone())
	{
		return false;
	}

	UMiniGameDefinition* Definition = FindMiniGameDefinition(MiniGameId);
	UWorld* World = GetWorld();
	if (Definition == nullptr || Definition->GameModeClass == nullptr || World == nullptr || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	if (Players.Num() < Definition->MinPlayers || Players.Num() > Definition->MaxPlayers)
	{
		return false;
	}

	if (!BuildSetupFromDefinition(Definition, Players))
	{
		return false;
	}

	ActiveDefinition = Definition;
	ReturnMapPackageName = World->GetPackage()->GetName();

	SetCurrentState(EMiniGameState::Preparing);
	OnMiniGamePreparing.Broadcast(ActiveSetup);

	SetCurrentState(EMiniGameState::Traveling);
	return TravelToMap(ActiveSetup.MiniGameMap);
}

bool UMiniGameManagerSubsystem::RequestFinishMiniGame()
{
	if (CurrentState != EMiniGameState::Playing)
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (AGameModeBase* GameMode = World->GetAuthGameMode())
		{
			if (IMiniGameInstanceInterface* MiniGameInstance = Cast<IMiniGameInstanceInterface>(GameMode))
			{
				MiniGameInstance->RequestFinishMiniGame();
				return true;
			}
		}
	}

	StopActiveMiniGame(EMiniGameFinishReason::Completed);
	return true;
}

void UMiniGameManagerSubsystem::StopActiveMiniGame(EMiniGameFinishReason Reason)
{
	if (CurrentState == EMiniGameState::Idle)
	{
		return;
	}

	if (CurrentState == EMiniGameState::Playing)
	{
		if (UWorld* World = GetWorld())
		{
			if (AGameModeBase* GameMode = World->GetAuthGameMode())
			{
				if (IMiniGameInstanceInterface* MiniGameInstance = Cast<IMiniGameInstanceInterface>(GameMode))
				{
					MiniGameInstance->StopMiniGame(Reason);
					return;
				}
			}
		}
	}

	FMiniGameResult Result;
	Result.MiniGameId = ActiveSetup.MiniGameId;
	Result.FinishReason = Reason;
	CommitMiniGameResult(Result);
}

void UMiniGameManagerSubsystem::CommitMiniGameResult(const FMiniGameResult& Result)
{
	if (CurrentState == EMiniGameState::Idle)
	{
		return;
	}

	SetCurrentState(EMiniGameState::Finishing);
	OnMiniGameFinished.Broadcast(Result);

	LastCommittedResult = Result;
	bHasLastCommittedResult = true;

	SetCurrentState(EMiniGameState::Completed);
	OnMiniGameResultCommitted.Broadcast(Result);

	const FString SavedReturnMapPackageName = ReturnMapPackageName;
	ClearActiveSession();

	if (!SavedReturnMapPackageName.IsEmpty())
	{
		SetCurrentState(EMiniGameState::Returning);

		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(SavedReturnMapPackageName);
		}
	}

	SetCurrentState(EMiniGameState::Idle);
}

void UMiniGameManagerSubsystem::NotifyMiniGameStarted()
{
	if (CurrentState != EMiniGameState::Preparing && CurrentState != EMiniGameState::Traveling)
	{
		return;
	}

	OnMiniGameStarted.Broadcast();
	SetCurrentState(EMiniGameState::Playing);
}

void UMiniGameManagerSubsystem::ClearLastCommittedResult()
{
	LastCommittedResult = FMiniGameResult();
	bHasLastCommittedResult = false;
}

UMiniGameDefinition* UMiniGameManagerSubsystem::FindMiniGameDefinition(FName MiniGameId)
{
	if (MiniGameId.IsNone())
	{
		return nullptr;
	}

	if (TObjectPtr<UMiniGameDefinition>* CachedDefinition = CachedDefinitions.Find(MiniGameId))
	{
		return CachedDefinition->Get();
	}

	CacheAvailableDefinitions();

	if (TObjectPtr<UMiniGameDefinition>* CachedDefinition = CachedDefinitions.Find(MiniGameId))
	{
		return CachedDefinition->Get();
	}

	return nullptr;
}

bool UMiniGameManagerSubsystem::IsActiveMiniGameWorld(const UWorld* World) const
{
	if (World == nullptr || !ActiveSetup.MiniGameMap.ToSoftObjectPath().IsValid())
	{
		return false;
	}

	const FString ActiveMiniGamePackageName = FPackageName::ObjectPathToPackageName(ActiveSetup.MiniGameMap.ToSoftObjectPath().ToString());
	return !ActiveMiniGamePackageName.IsEmpty() && World->GetPackage()->GetName() == ActiveMiniGamePackageName;
}

void UMiniGameManagerSubsystem::SetCurrentState(EMiniGameState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
	OnMiniGameStateChanged.Broadcast(CurrentState);
}

bool UMiniGameManagerSubsystem::BuildSetupFromDefinition(const UMiniGameDefinition* Definition, const TArray<APlayerState*>& Players)
{
	if (Definition == nullptr)
	{
		return false;
	}

	ActiveSetup = FMiniGameSetup();
	ActiveSetup.MiniGameId = Definition->MiniGameId;
	ActiveSetup.Genre = Definition->Genre;
	ActiveSetup.MinPlayers = Definition->MinPlayers;
	ActiveSetup.MaxPlayers = Definition->MaxPlayers;
	ActiveSetup.TimeLimitSeconds = Definition->DefaultTimeLimitSeconds;
	ActiveSetup.SessionTags = Definition->SessionTags;
	ActiveSetup.MiniGameMap = Definition->MiniGameMap;
	ActiveParticipants.Reset();

	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		if (Players[Index] == nullptr)
		{
			continue;
		}

		FMiniGameParticipantInfo Participant;
		Participant.PlayerState = Players[Index];
		Participant.SlotIndex = Index;
		Participant.Id = Index;
		Participant.bReady = true;
		ActiveParticipants.Add(Participant);
	}

	return ActiveParticipants.Num() >= Definition->MinPlayers && ActiveParticipants.Num() <= Definition->MaxPlayers;
}

void UMiniGameManagerSubsystem::ClearActiveSession()
{
	ActiveDefinition = nullptr;
	ActiveParticipants.Reset();
	ActiveSetup = FMiniGameSetup();
	ReturnMapPackageName.Reset();
}

bool UMiniGameManagerSubsystem::TravelToMap(const TSoftObjectPtr<UWorld>& MapAsset)
{
	UWorld* World = GetWorld();
	const FSoftObjectPath MapPath = MapAsset.ToSoftObjectPath();
	if (World == nullptr || World->GetNetMode() == NM_Client || !MapPath.IsValid())
	{
		ClearActiveSession();
		SetCurrentState(EMiniGameState::Idle);
		return false;
	}

	const FString TravelURL = FPackageName::ObjectPathToPackageName(MapPath.ToString());
	if (TravelURL.IsEmpty())
	{
		ClearActiveSession();
		SetCurrentState(EMiniGameState::Idle);
		return false;
	}

	World->ServerTravel(TravelURL);
	return true;
}

void UMiniGameManagerSubsystem::CacheAvailableDefinitions()
{
	CachedDefinitions.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> DefinitionAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UMiniGameDefinition::StaticClass()->GetClassPathName(), DefinitionAssets, true);

	for (const FAssetData& AssetData : DefinitionAssets)
	{
		UMiniGameDefinition* Definition = Cast<UMiniGameDefinition>(AssetData.GetAsset());
		if (Definition == nullptr || Definition->MiniGameId.IsNone())
		{
			continue;
		}

		CachedDefinitions.Add(Definition->MiniGameId, Definition);
	}
}
