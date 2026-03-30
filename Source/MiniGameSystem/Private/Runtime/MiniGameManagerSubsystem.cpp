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
	const FMiniGameNativeTags& Tags = FMiniGameNativeTags::Get();

	if (CurrentState != Tags.State_Idle || MiniGameId.IsNone())
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

	SetCurrentState(Tags.State_Preparing);
	OnMiniGamePreparing.Broadcast(ActiveSetup);

	SetCurrentState(Tags.State_Traveling);
	return TravelToMap(ActiveSetup.MiniGameMap);
}

bool UMiniGameManagerSubsystem::RequestFinishMiniGame()
{
	if (CurrentState != FMiniGameNativeTags::Get().State_Playing)
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

	StopActiveMiniGame(FMiniGameNativeTags::Get().FinishReason_Completed);
	return true;
}

void UMiniGameManagerSubsystem::StopActiveMiniGame(FGameplayTag Reason)
{
	const FMiniGameNativeTags& Tags = FMiniGameNativeTags::Get();

	if (CurrentState == Tags.State_Idle)
	{
		return;
	}

	if (CurrentState == Tags.State_Playing)
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
	const FMiniGameNativeTags& Tags = FMiniGameNativeTags::Get();

	if (CurrentState == Tags.State_Idle)
	{
		return;
	}

	SetCurrentState(Tags.State_Finishing);
	OnMiniGameFinished.Broadcast(Result);

	LastCommittedResult = Result;
	bHasLastCommittedResult = true;

	SetCurrentState(Tags.State_Completed);
	OnMiniGameResultCommitted.Broadcast(Result);

	const FString SavedReturnMapPackageName = ReturnMapPackageName;
	ClearActiveSession();

	if (!SavedReturnMapPackageName.IsEmpty())
	{
		SetCurrentState(Tags.State_Returning);

		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(SavedReturnMapPackageName);
		}
	}

	SetCurrentState(Tags.State_Idle);
}

void UMiniGameManagerSubsystem::NotifyMiniGameStarted()
{
	const FMiniGameNativeTags& Tags = FMiniGameNativeTags::Get();

	if (CurrentState != Tags.State_Preparing && CurrentState != Tags.State_Traveling)
	{
		return;
	}

	OnMiniGameStarted.Broadcast();
	SetCurrentState(Tags.State_Playing);
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

void UMiniGameManagerSubsystem::SetCurrentState(FGameplayTag NewState)
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
	ActiveSetup.MiniGameTags = Definition->MiniGameTags;
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
		SetCurrentState(FMiniGameNativeTags::Get().State_Idle);
		return false;
	}

	const FString TravelURL = FPackageName::ObjectPathToPackageName(MapPath.ToString());
	if (TravelURL.IsEmpty())
	{
		ClearActiveSession();
		SetCurrentState(FMiniGameNativeTags::Get().State_Idle);
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
