#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/*
GameplayTag 네이밍 규칙

- 기본 형식 : Domain.Category.Value
  예시: MiniGame.State.Playing

- 각 세그먼트는 PascalCase로 작성.
  예시: MiniGame, FinishReason, TimeOver

- 태그 경로 문자열은 영문으로 유지.
  한글은 _COMMENT 설명 문자열에만 사용.

- 같은 의미축의 값들은 반드시 같은 부모 태그 아래에 모아야 함.
- 진행 상태처럼 상호 배타적인 값은 공통 Category 아래에 정리한다.
  예시: MiniGame.State.Idle, MiniGame.State.Playing
  오사용 예시: MiniGame.Playing 과 MiniGame.State.Playing 을 동시에 사용하지 않는 것이 중요.
  형식: MiniGame.State.*

- 당연히 장르, 종료 사유, 이벤트도 각각 별도 Category 로 분리.
  예시: MiniGame.Genre.*, MiniGame.FinishReason.*, MiniGame.Event.*
*/

struct MINIGAMESYSTEM_API FMiniGameNativeTags
{
	static const FMiniGameNativeTags& Get();
	static void InitializeNativeTags();

	FGameplayTag State_Idle;
	FGameplayTag State_Preparing;
	FGameplayTag State_Traveling;
	FGameplayTag State_Playing;
	FGameplayTag State_Finishing;
	FGameplayTag State_Returning;
	FGameplayTag State_Completed;

	FGameplayTag Genre_None;
	FGameplayTag Genre_Competition;
	FGameplayTag Genre_Survival;
	FGameplayTag Genre_Race;

	FGameplayTag FinishReason_TimeOver;
	FGameplayTag FinishReason_Completed;

private:
	FMiniGameNativeTags();
};
