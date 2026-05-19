# Code Quality Report — `TVController` / `pushButton()`

> **리뷰 일자**: 2026-05-19  
> **대상 파일**: `include/TVController.h`, `include/remoteKey.h`  
> **리뷰 관점**: SOLID, Code Smell, C++17 모던 스타일

---

## 0. 분석 범위 안내

요청하신 **`updateQuality()`는 현재 저장소에 존재하지 않습니다** (`git grep`, 전체 `.h/.cpp` 검색 결과 없음).

동일한 아키텍처·스멜 패턴을 보이는 **실질 진입점 `pushButton()`** 및 그가 조율하는 private 메서드군(숫자 버퍼, 선호 채널, 검색 목록, 업/다운)을 분석 대상으로 삼았습니다.  
사용자가 언급한 매직 넘버 **0, 50, 11, 6**은 다음과 같이 매핑합니다.

| 값 | 출처 | 의미 |
|----|------|------|
| `0`, `99`, `100` | `TVController.h` 구현 | 채널 최소/최대, 순환 모듈로 |
| `2` | `handleDigit` | 2자리 즉시 커밋 임계값 |
| `6` | README §5·§6, 테스트 | 대표 시나리오 채널 (구현 리터럴은 아님) |
| `50` | `TVControllerTest` 등 | 경계 테스트용 채널 (구현 리터럴은 아님) |
| `11` | — | **구현·명세에 없음** — `12`(선호 next) 또는 `99`(wrap)와 혼동 가능 |

---

## 1. 문제점 분석 표

| 문제점 | 위반 원칙 / 스멜 | 영향 | 개선 방향 | 우선순위 |
|--------|------------------|------|-----------|----------|
| `TVController`가 입력 디스패치·숫자 FSM·채널 검증·선호 목록·검색 수집·3종 네비게이션·`std::cout` 로깅을 한 클래스에서 처리 | **SRP** 위반, **God Class** | 변경 시 회귀 범위가 넓고, 단위 테스트가 “컨트롤러 전체”에 의존 | `DigitInputHandler`, `FavoriteStore`, `SearchChannelNavigator` 등 역할별 타입 분리; `TVController`는 조합·위임만 | **4** |
| `pushButton()`의 `switch (key)` — 새 키 추가 시 분기 수정 필수 | **OCP** 위반 | 키 enum 확장마다 컨트롤러 수정·재컴파일 | `std::unordered_map<remoteKey, KeyHandler>` 또는 `std::array` + 함수 포인터/`std::function` 테이블 디스패치 | **3** |
| `channelUpWithSearch` / `channelDownWithSearch` / `handleFavNext`의 “정렬 목록에서 current 기준 next/prev + wrap” 로직 3중 복제 | **Duplicated Code**, **Shotgun Surgery** | 한쪽만 버그 수정 시 나머지 불일치 (wrap·빈 목록·경계) | `findNextInSorted(int current, const std::vector<int>&, Direction)` 단일 알고리즘 추출; `std::optional<int>` 반환 | **1** |
| `channelUpWithoutSearch` / `channelDownWithoutSearch`의 `(ch±1+100)%100` 대칭 패턴 | **Duplicated Code** | up/down 규칙 변경 시 2곳 수정 | `ChannelArithmetic::step(int ch, int delta)` 또는 `enum class Step { Up, Down }` | **2** |
| 리터럴 `0`, `99`, `100`, `2`, `-1`, `+99` 산재 | **Magic Number** | 도메인 변경(예: 0~999 채널) 시 누락·오타 위험 | `namespace tv { inline constexpr int kMinChannel = 0; kMaxChannel = 99; kChannelCount = 100; kMaxDigitLen = 2; }` | **2** |
| `commitChannel` / `getCurrentChannelInt` / `handleSearch`에서 반복 `std::stoi`·`std::to_string` | **Duplicated Code**, 비효율 | 예외 지점 분산, 문자열↔정수 변환 비용 | 내부 상태를 `int currentChannel_`로 유지하고 `Tuner` 경계에서만 문자열 변환 | **5** |
| `pushButton` + 7개 `handle*` + 4개 `channel*` — 헤더 단일 파일 200+ LOC | **Long Method** (클래스 단위), **Header Bloat** | 컴파일 의존성·가독성 저하 | 구현을 `TVController.cpp`로 분리; public API만 헤더 노출 | **5** |
| `searchedChannels.empty()` / `favoriteChannels.empty()` 분기가 여러 메서드에 분산 | **높은 조건문 복잡도**, **OCP** | 업/다운·next 동작 모드 추가 시 분기 폭증 | `std::variant<LinearNav, ListNav>` 또는 Strategy 인터페이스 `IChannelNavigator` | **3** |
| `next = -1` / `prev = -1` 센티널 | **Magic Number**, 취약한 관례 | `-1`이 유효 채널이 되면 침묵 버그 | `std::optional<int>` + `value_or(wrapTarget)` | **1** |
| `clearBufferBeforeNonDigitAction()` → `clearBuffer()` 1줄 위임 | **Indirection / needless wrapper** | 호출 스택만 깊어짐 | `clearBuffer()` 직접 호출 또는 매크로성 이름을 인라인 정책으로 통합 | **5** |
| `commitChannel` 내 `std::cout` | **SRP**, 테스트 결합 | Golden/단위 테스트에서 관측 불가 출력 혼재 | `IChannelSink` / 로거 주입 또는 제거(요구사항상 비관측) | **4** |
| raw `Tuner*` 소유 없음, null 검사 없음 | **안전성** (SOLID 외) | 잘못된 주입 시 UB | `Tuner&` 참조 또는 `std::not_null<Tuner*>` (C++20) / 생성자 검증 | **5** |
| `remoteKey.h`의 `to_string` 거대 `switch` | **OCP**, **Duplicated Code** (키마다 case) | 키 추가 시 2곳 수정 (`enum` + `switch`) | `constexpr std::array<std::string_view, N>` 또는 X-Macro로 enum·문자열 동기화 | **3** |

---

## 2. SRP / OCP 위반 — 근거 상세

### 2.1 SRP (Single Responsibility Principle)

`TVController`는 최소 **6가지 변경 이유**를 동시에 가집니다.

1. **리모컨 키 라우팅** — `pushButton`, digit vs non-digit 분기  
2. **숫자 입력 상태기계** — `processingCH`, 2자리 커밋, OK 확정  
3. **채널 도메인 규칙** — 0~99 검증, `commitChannel`  
4. **선호 채널 집합** — toggle, sort, FAV_NEXT  
5. **검색 목록 수집** — `seekCH` 루프  
6. **채널 네비게이션 정책** — linear wrap / search list / favorite list  

한 요구사항(예: “선호 next만 wrap 규칙 변경”)이 다른 영역(숫자 버퍼)까지 건드릴 위험은 낮지만, **테스트·리뷰 단위가 비대**해져 SRP 위반이 실질적입니다.

### 2.2 OCP (Open-Closed Principle)

- **키 확장**: `remoteKey`에 값 추가 → `pushButton`의 `switch`에 `case` 추가 (닫혀 있지 않음).  
- **네비게이션 모드 확장**: 예) “최근 시청 채널” 모드 추가 시 `handleChannelUp/Down`의 `if (searchedChannels.empty())` 분기와 별도 알고리즘 메서드 추가가 필요.  
- **공통 패턴 미추상화**: “정렬된 `vector<int>`에서 current 기준 순환 이동”이 3곳에 복사되어, 정책 수정 시 **3곳을 열어야** 합니다 → 확장에 닫혀 있지 않음.

---

## 3. Magic Number 상수화 필요성

| 리터럴 | 위치 (대표) | 도메인 의미 | 상수화 제안 |
|--------|-------------|-------------|-------------|
| `0` | `commitChannel` 하한, wrap 결과 | 최소 채널 | `kMinChannel` |
| `99` | `commitChannel` 상한, `+99` down | 최대 채널 | `kMaxChannel` |
| `100` | `% 100` wrap | 채널 개수 | `kChannelCount` |
| `2` | `processingCH.size() >= 2` | 숫자 버퍼 최대 자릿수 | `kMaxDigitBufferLen` |
| `99` (in `+ 99`) | `channelDownWithoutSearch` | down 1 step in mod 100 | `kChannelCount - 1` 또는 `step(ch, -1)` |
| `-1` | next/prev 미발견 | 센티널 | `std::nullopt` |
| `'0'` | `digitFromKey` | ASCII 오프셋 | `kDigitAsciiBase` (remoteKey.h) |

**0, 6, 50, 11** 중 **6·50·11은 테스트·README 시나리오 값**이며 구현에 하드코딩되어 있지 않습니다.  
다만 **0·99·100·2**는 프로덕션 코드에 직접 박혀 있으므로 **우선 상수화 대상**입니다.

---

## 4. Code Smell 요약

| 스멜 | 해당 코드 | 심각도 |
|------|-----------|--------|
| **Duplicated Code** | `channelUpWithSearch` ≈ `handleFavNext`; `channelDownWithSearch` 역방향 동일 | 높음 |
| **Long Method** (집계) | 헤더 내 전 메서드 인라인 (~200 LOC) | 중간 |
| **Switch Statements** | `pushButton`, `remoteKey::to_string` | 중간 |
| **Feature Envy** | 네비게이션이 `favoriteChannels`/`searchedChannels` 내부 순회에 집중 | 중간 |
| **Primitive Obsession** | 채널을 `std::string`으로만 다루고 매번 `stoi` | 중간 |
| **Message Chains** | `getCurrentChannelInt()` → `tuner->getCurrentCH()` → `stoi` | 낮음 |

`pushButton()` 자체는 약 30줄로 **단일 메서드 Long Method는 아니나**, 실질 복잡도는 위임된 10+ 메서드에 누적되어 **클래스 수준 Long Method**로 보는 것이 타당합니다.

---

## 5. C++17 개선 방향

### 5.1 테이블 기반 키 디스패치 (OCP)

```cpp
using KeyHandler = void (TVController::*)();
static const std::unordered_map<remoteKey, KeyHandler> kHandlers{
    {remoteKey::KEY_OK, &TVController::handleOk},
    {remoteKey::KEY_CH_UP, &TVController::handleChannelUp},
    // ...
};
if (auto it = kHandlers.find(key); it != kHandlers.end()) {
    (this->*it->second)();
}
```

새 키는 **맵 등록만**으로 확장 가능 (기존 `switch` 수정 최소화).

### 5.2 `std::variant` / Strategy — 네비게이션 모드

```cpp
enum class NavMode { Linear, SearchList, FavoriteList };
std::variant<LinearNavigator, ListNavigator> channelNav_;
```

`handleChannelUp`은 `std::visit`으로 위임 → `searchedChannels.empty()` 분기 제거.

### 5.3 공통 알고리즘 + `std::optional`

```cpp
enum class Direction { Forward, Backward };
std::optional<int> findNeighbor(int current,
    const std::vector<int>& sorted, Direction dir);
```

3중 복제 제거, C++17 `std::optional`로 센티널 `-1` 제거.

### 5.4 `constexpr` 상수 (C++17)

```cpp
namespace tv::channel {
inline constexpr int kMin = 0;
inline constexpr int kMax = 99;
inline constexpr int kCount = kMax - kMin + 1;
}
```

### 5.5 적용 우선순위 (전략 vs variant)

| 접근 | 적합한 경우 | 이 프로젝트 |
|------|-------------|-------------|
| **테이블 디스패치** | 키 종류 고정·동작만 추가 | ✅ `pushButton`에 즉시 효과 |
| **공통 함수 추출** | 알고리즘 동일·컨테이너만 다름 | ✅ **최우선** (3곳 중복) |
| **Strategy / variant** | 네비게이션 모드가 런타임에 바뀜 | △ 현재는 검색 유무만 분기 — 중기 리팩터 |
| **전략 패턴 클래스 계층** | 모드 5개 이상·플러그인 | △ 현재 3모드 — 과도할 수 있음 |

---

## 6. 리팩토링 우선순위 (1~5)

| 순위 | 작업 | 이유 |
|------|------|------|
| **1** | 정렬 목록 기준 next/prev + wrap **단일 함수** 추출 | 버그 가능성 가장 큰 **3중 중복** 제거; 테스트 1곳으로 수렴 |
| **2** | 채널 도메인 상수 (`0/99/100/2`) 및 `stepChannel(ch, ±1)` | 저비용·고수익; wrap/down의 `+99` 의도 명시화 |
| **3** | `pushButton` 테이블 디스패치 + (선택) `remoteKey::to_string` 테이블화 | **OCP** 개선; 키 추가 비용 감소 |
| **4** | 역할별 클래스 분리 + `cout` 제거/주입 | **SRP**; 장기 유지보수·테스트 격리 |
| **5** | `.cpp` 분리, `int` 내부 채널 표현, thin wrapper 제거 | 구조 정리·컴파일 시간; 기능 동작 변경 없음 |

---

## 7. 개선 방향 요약

현재 `TVController`는 **기능적으로는 요구사항을 충족**하지만, **하나의 헤더 클래스에 입력 FSM·3종 채널 이동·선호·검색이 결합**되어 SRP/OCP 측면에서 확장 비용이 큽니다.  
가장 시급한 것은 **매직 넘버 `-1`과 중복된 “정렬 벡터 순환 탐색”**이며, C++17에서는 `std::optional`과 `constexpr` 상수, 단기적으로 **공통 함수 + 키 핸들러 테이블**이 비용 대비 효과가 가장 큽니다.

`updateQuality()`가 향후 추가될 경우, **화질 등급(0~50 등) 계산을 `TVController`에 넣지 말고** 별도 `PictureQualityPolicy`로 분리하고, `pushButton`에는 테이블 엔트리만 추가하는 구조를 권장합니다. 그렇지 않으면 본 문서에서 지적한 God Class·`switch` OCP 문제가 **그대로 재발**합니다.

---

## 8. 참고 코드 위치

| 관심사 | 파일 | 라인(대략) |
|--------|------|------------|
| 키 디스패치 | `include/TVController.h` | 177–205 |
| 중복 next 로직 | 동일 | 83–96, 155–171, 98–110 |
| 채널 wrap | 동일 | 73–80, 39–40 |
| 매직 `2` | 동일 | 57–58 |
| digit 판별 | `include/remoteKey.h` | 57–63 |

---

*문서 버전: 1.0 — `include/TVController.h`, `include/remoteKey.h` 기준 정적 리뷰*
