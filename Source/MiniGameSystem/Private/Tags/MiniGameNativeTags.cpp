#include "Tags/MiniGameNativeTags.h"

#include "NativeGameplayTags.h"

namespace MiniGameGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_State_Idle, "MiniGame.State.Idle", "미니게임 세션이 없고 대기 중인 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_State_Preparing, "MiniGame.State.Preparing", "미니게임 시작을 준비하는 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_State_Traveling, "MiniGame.State.Traveling", "플레이어가 미니게임 맵으로 이동 중인 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_State_Playing, "MiniGame.State.Playing", "미니게임이 현재 진행 중인 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_State_Finishing, "MiniGame.State.Finishing", "미니게임 결과를 정리하고 종료 처리 중인 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_State_Returning, "MiniGame.State.Returning", "플레이어가 원래 맵으로 복귀 중인 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_State_Completed, "MiniGame.State.Completed", "미니게임이 완료된 상태");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_Genre_None, "MiniGame.Genre.None", "지정되지 않은 미니게임 장르");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_Genre_Competition, "MiniGame.Genre.Competition", "경쟁 미니게임 장르");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_Genre_Survival, "MiniGame.Genre.Survival", "생존 미니게임 장르");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_Genre_Race, "MiniGame.Genre.Race", "레이스 미니게임 장르");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_FinishReason_TimeOver, "MiniGame.FinishReason.TimeOver", "제한 시간이 종료되어 미니게임이 끝난 경우");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MiniGame_FinishReason_Completed, "MiniGame.FinishReason.Completed", "승리 조건이나 완료 조건을 충족해 미니게임이 끝난 경우");
}

FMiniGameNativeTags::FMiniGameNativeTags()
	: State_Idle(MiniGameGameplayTags::TAG_MiniGame_State_Idle)
	, State_Preparing(MiniGameGameplayTags::TAG_MiniGame_State_Preparing)
	, State_Traveling(MiniGameGameplayTags::TAG_MiniGame_State_Traveling)
	, State_Playing(MiniGameGameplayTags::TAG_MiniGame_State_Playing)
	, State_Finishing(MiniGameGameplayTags::TAG_MiniGame_State_Finishing)
	, State_Returning(MiniGameGameplayTags::TAG_MiniGame_State_Returning)
	, State_Completed(MiniGameGameplayTags::TAG_MiniGame_State_Completed)
	, Genre_None(MiniGameGameplayTags::TAG_MiniGame_Genre_None)
	, Genre_Competition(MiniGameGameplayTags::TAG_MiniGame_Genre_Competition)
	, Genre_Survival(MiniGameGameplayTags::TAG_MiniGame_Genre_Survival)
	, Genre_Race(MiniGameGameplayTags::TAG_MiniGame_Genre_Race)
	, FinishReason_TimeOver(MiniGameGameplayTags::TAG_MiniGame_FinishReason_TimeOver)
	, FinishReason_Completed(MiniGameGameplayTags::TAG_MiniGame_FinishReason_Completed)
{
}

const FMiniGameNativeTags& FMiniGameNativeTags::Get()
{
	static const FMiniGameNativeTags Tags;
	return Tags;
}

void FMiniGameNativeTags::InitializeNativeTags()
{
	Get();
}
