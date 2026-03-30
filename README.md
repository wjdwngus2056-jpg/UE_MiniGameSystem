# MiniGameSystem

`MiniGameSystem`은 미니게임을 실행하고 해당하는 미니게임 맵으로 이동하여 미니게임을 진행한 뒤, 게임 결과를 호출 측으로 되돌려주는 런타임 플러그인입니다.

해당 플러그인의 시작은 멀티플레이 팀프로젝트에서 쉽게 미니게임을 추가하기 위해서 모듈의 형태로 제작했으나, 비슷한 형식의 다른 게임에서도 재사용할 수 있을 것 같아서 플러그인으로 만들었습니다.

## 핵심 구성
### 필수
- `UMiniGameDefinition`
  - 미니게임 `Id`, 맵, 플레이어 수, 게임 모드를 등록하는 데이터 에셋입니다.
  - 선택적으로 룰셋을 만들어서 등록할 수도 있습니다.
- `UMiniGameManagerSubsystem`
  - 플러그인의 메인 진입점입니다. 
  - 미니게임 시작, 활성 세션 관리, 결과 커밋, 이전 맵 복귀를 담당합니다.
- `AMiniGameGameModeBase`
  - 각 미니게임 맵에서 사용하는 기본 게임 모드입니다. 
  - 매니저에서 전달된 세션 정보를 자동으로 받아 초기화하고, 종료 시 결과를 자동으로 커밋합니다.
- `AMiniGameGameStateBase`
  - 참가자, 남은 시간, 점수판을 복제하는 공통 게임 상태 클래스입니다.
### 선택
- `UMiniGameRuleSet`
  - 점수 처리, 순위 계산, 결과 생성 정책을 정의하는 선택적 규칙 객체입니다.
  - 미니게임이 공통된 점수계산방식이나 규칙을 사용하는 일이 많아질 때 `RuleSet`을 만드는 것을 고려해볼 만 합니다.

## 기본적인 사용법

1. 에디터에서 `UMiniGameDefinition` 에셋을 만든다.
2. `MiniGameId`, `MiniGameMap`, `GameModeClass`, 플레이어 수를 필수적으로 설정한다.
3. 미니게임 맵의 게임 모드를 `AMiniGameGameModeBase` 파생 클래스로 지정한다.
4. 미니게임을 시작하고 싶을 때 `RequestStartMiniGame`을 호출한다.
5. 미니게임 내부에서는 `AddScore`, `FinishMiniGame`를 사용해서 점수를 계산하고 게임을 종료한다.

## 미니게임 시작 방법

```cpp
if(UMiniGameManagerSubsystem* MiniGameManager = GetGameInstance()->GetSubsystem<UMiniGameManagerSubsystem>())
{
    MiniGameManager->RequestStartMiniGame(TEXT("TapGame"), Players);
}
```

`RequestStartMiniGame`을 호출하면 아래 작업이 자동으로 수행됩니다.

- `MiniGameId`에 맞는 `UMiniGameDefinition` 조회
- `FMiniGameSetup` 구성
- 현재 맵을 복귀용 맵으로 저장
- 미니게임 맵으로 `ServerTravel`

### 주의
`RequestStartMiniGame`은 서버(메인 GameMode)에서 호출해야합니다.
```cpp
UWorld* World = GetWorld();
if (Definition == nullptr || Definition->GameModeClass == nullptr || World == nullptr || World->GetNetMode() == NM_Client)
{
    return false;
}
```
`World->GetNetMode() == NM_Client` 검사 때문에 클라이언트라면 false를 반환하므로 클라이언트에서 호출하면 실패합니다.

## 미니게임 작성 방법

`AMiniGameGameModeBase`를 상속한 게임 모드를 하나 만들고 내부적으로 로직을 작성하면 됩니다.

필수적으로 사용해야하는 목록은 아래와 같습니다.

- 점수계산이 필요한 시점에 `AddScore(PlayerState, DeltaScore)` 호출
- 게임 종료 조건이 만족되면 `FinishMiniGame(Reason)` 호출

플러그인에서 기본적으로 처리해주는 것은 아래와 같습니다.

- `UMiniGameManagerSubsystem`에서 세션 정보 가져오기
- `AMiniGameGameStateBase` 초기화
- 남은 시간 갱신
- 선택된 `UMiniGameRuleSet` 생성 및 적용
- 최종 `FMiniGameResult` 생성 및 커밋

## 결과 받는 방법

`UMiniGameManagerSubsystem`의 델리게이트에 바인딩해서 결과를 받을 수 있습니다.
UI를 비롯하여 기타 필요한 순간에 사용하면 됩니다.

- `OnMiniGamePreparing` : 미니게임 준비중
- `OnMiniGameStarted` : 미니게임 시작됨
- `OnMiniGameFinished` : 미니게임 종료됨
- `OnMiniGameResultCommitted` : 결과 커밋

마지막으로 커밋된 결과는 아래 함수로 직접 조회할 수도 있습니다.

- `GetLastCommittedResult()`
- `HasLastCommittedResult()`

## 프로젝트에서 미니게임을 만들 때

새 미니게임을 추가할 때 직접 건드려야 하는 부분을 아래 3개로 한정하려고 노력했습니다. 따라서 그 외의 실행 흐름, 상태 관리, 결과 커밋, 복귀 처리는 가능한 한 모두 플러그인 내부에서 해결할 수 있으므로 따로 작성하지 않아도 됩니다.

- `UMiniGameDefinition` 에셋 1개
- 미니게임 맵 1개
- `AMiniGameGameModeBase` 파생 클래스 1개 : 로직작성

## MiniGameRuleSet 사용법

`MiniGameGameModeBase`에서 로직을 구성해도 작동하기 때문에 `RuleSet`을 굳이 사용할 필요는 없습니다. 다만 아래처럼 사용을 고려해야하는 경우가 있을 수 있습니다.
- 여러 미니게임이 같은 점수 계산방식을 공유하는 경우

`MiniGameRuleSet`으로 규칙을 설정한 뒤, `UMinigameDefinition`에서 `RuleSetClass`를 찾아 연결하면 `RuleSet`이 비어있지 않은 경우에 `GameMode`에서는 `RuleSet`의 함수들을 우선적으로 호출합니다.