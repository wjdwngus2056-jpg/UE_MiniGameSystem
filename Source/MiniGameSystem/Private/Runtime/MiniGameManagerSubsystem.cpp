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

	// 시작 시 미니게임 정의 목록을 읽고, 이전 실행 흔적을 초기화
	CacheAvailableDefinitions();
	ClearLastCommittedResult();
	ClearActiveSession();
}

void UMiniGameManagerSubsystem::Deinitialize()
{
	// 종료 시 런타임 캐시와 진행 정보를 정리
	CachedDefinitions.Reset();
	ClearLastCommittedResult();
	ClearActiveSession();

	Super::Deinitialize();
}

bool UMiniGameManagerSubsystem::RequestStartMiniGame(FName MiniGameId, const TArray<APlayerState*>& Players)
{
	const FMiniGameNativeTags& Tags = FMiniGameNativeTags::Get();

	// 대기 상태가 아니거나 ID가 없으면 새 미니게임을 시작할 수 없다
	if (CurrentState != Tags.State_Idle || MiniGameId.IsNone())
	{
		return false;
	}

	UMiniGameDefinition* Definition = FindMiniGameDefinition(MiniGameId);
	UWorld* World = GetWorld();
	
	// World->GetNetMode() == NM_Client 인 경우에 false를 return 하므로 클라이언트에서 사용할 수 없음. 반드시 서버에서 사용
	if (Definition == nullptr || Definition->GameModeClass == nullptr || World == nullptr || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	// 정의된 허용 인원 범위를 벗어나면 미니게임을 시작할 수 없음
	if (Players.Num() < Definition->MinPlayers || Players.Num() > Definition->MaxPlayers)
	{
		return false;
	}

	// 선택한 정의와 참가자 목록을 실제 실행 데이터로 변환
	if (!BuildSetupFromDefinition(Definition, Players))
	{
		return false;
	}

	ActiveDefinition = Definition;
	// 미니게임 종료 후 되돌아올 월드 이름을 저장해둠.
	ReturnMapPackageName = World->GetPackage()->GetName();

	SetCurrentState(Tags.State_Preparing);
	OnMiniGamePreparing.Broadcast(ActiveSetup);

	SetCurrentState(Tags.State_Traveling);
	return TravelToMap(ActiveSetup.MiniGameMap);
}

bool UMiniGameManagerSubsystem::RequestFinishMiniGame()
{
	// 실제 게임 종료 판정은 현재 미니게임의 GameMode에 위임
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
	// 플레이 중인 GameMode가 없는 경우에 최소한의 결과만 만들어 직접적으로 종료처리.
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
			// 미니게임이 끝나면 서버가 참가자 전원은 원래 맵으로 복귀.
			World->ServerTravel(SavedReturnMapPackageName);
		}
	}

	SetCurrentState(Tags.State_Idle);
}

void UMiniGameManagerSubsystem::NotifyMiniGameStarted()
{
	const FMiniGameNativeTags& Tags = FMiniGameNativeTags::Get();

	// 맵 이동 후에만 Playing 상태로 진입할 수 있어야 하기 때문에 Preparing, Traveling 상태가 아니라면 return
	if (CurrentState != Tags.State_Preparing && CurrentState != Tags.State_Traveling)
	{
		return;
	}

	OnMiniGameStarted.Broadcast();
	SetCurrentState(Tags.State_Playing);
}

void UMiniGameManagerSubsystem::ClearLastCommittedResult()
{
	// 이전 결과를 지워 다음 미니게임 실행과 구분할 필요가 있기 때문에 만든 함수
	LastCommittedResult = FMiniGameResult();
	bHasLastCommittedResult = false;
}

UMiniGameDefinition* UMiniGameManagerSubsystem::FindMiniGameDefinition(FName MiniGameId)
{
	if (MiniGameId.IsNone())
	{
		return nullptr;
	}
	
	// 우선 캐시된 정의에서 찾고
	if (TObjectPtr<UMiniGameDefinition>* CachedDefinition = CachedDefinitions.Find(MiniGameId))
	{
		return CachedDefinition->Get();
	}
	
	// 없으면 에셋 목록을 다시 스캔
	CacheAvailableDefinitions();

	if (TObjectPtr<UMiniGameDefinition>* CachedDefinition = CachedDefinitions.Find(MiniGameId))
	{
		return CachedDefinition->Get();
	}

	return nullptr;
}

bool UMiniGameManagerSubsystem::IsActiveMiniGameWorld(const UWorld* World) const
{
	// 현재 월드가 선택된 미니게임 맵과 같은지 확인
	if (World == nullptr || !ActiveSetup.MiniGameMap.ToSoftObjectPath().IsValid())
	{
		return false;
	}

	const FString ActiveMiniGamePackageName = FPackageName::ObjectPathToPackageName(ActiveSetup.MiniGameMap.ToSoftObjectPath().ToString());
	return !ActiveMiniGamePackageName.IsEmpty() && World->GetPackage()->GetName() == ActiveMiniGamePackageName;
}

void UMiniGameManagerSubsystem::SetCurrentState(FGameplayTag NewState)
{
	// 상태가 바뀔 때만 브로드캐스트해서 UI나 로직이 중복 반응하지 않게 하는 로직
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
		
		// 참가자 정보는 맵 이동 후 GameMode와 GameState를 초기화할 때 재사용
		ActiveParticipants.Add(Participant);
	}

	return ActiveParticipants.Num() >= Definition->MinPlayers && ActiveParticipants.Num() <= Definition->MaxPlayers;
}

void UMiniGameManagerSubsystem::ClearActiveSession()
{
	// 현재 진행 중인 미니게임과 관련된 런타임 데이터만 Clear
	ActiveDefinition = nullptr;
	ActiveParticipants.Reset();
	ActiveSetup = FMiniGameSetup();
	ReturnMapPackageName.Reset();
}

bool UMiniGameManagerSubsystem::TravelToMap(const TSoftObjectPtr<UWorld>& MapAsset)
{
	UWorld* World = GetWorld();
	const FSoftObjectPath MapPath = MapAsset.ToSoftObjectPath();
	// 잘못된 맵이거나 클라이언트 월드라면 이동을 중단하고 상태를 초기화
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

	// 미니게임 진입은 클라이언트가 아니라 서버의 맵 전환으로 처리
	World->ServerTravel(TravelURL);
	return true;
}

void UMiniGameManagerSubsystem::CacheAvailableDefinitions()
{
	CachedDefinitions.Reset();

	// AssetRegistry를 쓴 이유 : UMiniGameDefinition 데이터 에셋들을 프로젝트 전체에서 자동으로 찾아오기 위함
	// 데이터 에셋을 계속 추가하면서 미니게임을 수를 확장해나가야하는 구조상 자동으로 가져오는 것이 구조적으로 적합하다고 판단
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> DefinitionAssets;
	// 프로젝트에 존재하는 모든 미니게임 정의 에셋을 찾아서 저장함.
	AssetRegistryModule.Get().GetAssetsByClass(UMiniGameDefinition::StaticClass()->GetClassPathName(), DefinitionAssets, true);

	for (const FAssetData& AssetData : DefinitionAssets)
	{
		// AssetData는 메타데이터이므로 실제 정의 객체로 캐스팅해서 사용.
		UMiniGameDefinition* Definition = Cast<UMiniGameDefinition>(AssetData.GetAsset());
		if (Definition == nullptr || Definition->MiniGameId.IsNone())
		{
			continue;
		}

		CachedDefinitions.Add(Definition->MiniGameId, Definition);
	}
}
