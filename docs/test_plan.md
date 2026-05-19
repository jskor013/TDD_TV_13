# TV_TDD 테스트 계획서

> **역할**: 시니어 QA 리드  
> **대상**: `TVController` (C++17), `Tuner` Mock/Fake 기반 단위·통합 테스트  
> **근거**: [requirements_analysis.md](./requirements_analysis.md), `README.md`, `include/TVController.h`, `test/TunerTest.cpp`  
> **도구**: Google Test / Google Mock, CMake, (권장) gcov + lcov  
> **버전**: 1.0

---

## 1. 목적 및 범위

### 1.1 목적

리모컨 입력(`pushButton(remoteKey)`) 시퀀스에 대해 **채널 변경 결과**와 **`Tuner` 호출 순서·인자**가 요구사항과 일치하는지 자동화 테스트로 검증한다. 로그(`std::cout`)는 비관측 대상이다.

### 1.2 In Scope

| 영역 | REQ-ID (참고) |
|------|----------------|
| 숫자 버튼 채널 변경 (버퍼, 2자리 커밋, 선행 0, 3번째 숫자 대기) | REQ-CH-01 ~ 05 |
| 키 1회 입력당 동작 | §2 (요구사항 분석) |
| 선호 채널 추가/삭제 (토글) | REQ-FAV-01 |
| 다음 선호 채널 (단일·연속 입력) | REQ-FAV-02 |
| 선호 목록 없음 | REQ-FAV-02 (정책) |
| 채널 검색 목록 구축 | REQ-SEEK-01 |
| 채널 업/다운 (검색 유/무, 경계 래핑) | REQ-UD-01, REQ-UD-02 |
| 숫자 버퍼 vs 기타 키 충돌 | REQ-CH-05 |
| 채널 유효 범위 0~99 (컨트롤러 경유) | §0.1 |

### 1.3 Out of Scope

- `Tuner` 구현체 내부 로직 (제공 API 계약만 Mock으로 검증)
- 키 홀드·동시 입력·UI/로그 출력
- `TunerTest.cpp` 수정 (기존 계약 테스트 유지)

### 1.4 테스트 정책 (구현 전 합의 필요)

| ID | 미정 항목 | **테스트 계획 권장 기대** | 검증 방법 |
|----|-----------|---------------------------|-----------|
| POL-01 | 빈 버퍼에서 `OK` | no-op, `setCH` 미호출 | Mock `Times(0)` |
| POL-02 | 선호 목록 비어 있을 때 `FAV_NEXT` | no-op, `setCH` 미호출 | Mock `Times(0)` |
| POL-03 | `setCH`에 무효 채널 전달 시 | `invalid_argument` 전파 또는 catch 후 버퍼 초기화 | `EXPECT_THROW` 또는 상태 검증 |
| POL-04 | `FAV_NEXT` 연속 입력 | 매 press마다 알고리즘 재적용 (§5.2) | 연속 시퀀스 테이블 |

> 구현이 정책과 다르면 **문서·코드 중 하나를 갱신**하고 해당 케이스를 Characterization 테스트로 고정한다.

---

## 2. 테스트 환경

### 2.1 빌드·실행

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### 2.2 커버리지 (gcov / lcov, 선택)

CMake 예시 플래그:

```cmake
# 루트 CMakeLists.txt 또는 옵션
option(ENABLE_COVERAGE "Build with coverage" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
  add_link_options(--coverage)
endif()
```

실행 후 리포트:

```bash
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*googletest*' --output-file coverage.filtered.info
genhtml coverage.filtered.info --output-directory build/coverage_html
```

**커버리지 목표 (권장)**:

| 대상 | Line | Branch (가능 시) |
|------|------|------------------|
| `TVController` 구현 | ≥ 90% | ≥ 85% |
| `pushButton` 분기 (키 종류·상태) | 100% (키 enum 전체) | — |

### 2.3 테스트 더블

| 더블 | 용도 | 파일 |
|------|------|------|
| `MockTunerForController` | `setCH` / `getCurrentCH` / `seekCH` 기대값·순서 | `test/TVControllerTest.cpp` |
| `TunerTest` Mock | 채널 유효/무효 계약 (참고, 비수정) | `test/TunerTest.cpp` |

**Mock 원칙**

1. 관측: `setCH(인자)`, 호출 횟수, `getCurrentCH` 반환 시퀀스, `seekCH` 반복.
2. 시퀀스 테스트: `::testing::InSequence`로 호출 순서 고정.
3. 초기 채널: 미지정 시 `"0"` 또는 테스트별 `ON_CALL`로 명시.

---

## 3. 테스트 설계 원칙

### 3.1 명명 규칙

```
TEST_F(TVControllerFixture, REQ_<영역>_<번호>_<요약>)
// 예: TEST_F(TVControllerFixture, REQ_CH_01_OneDigitThenOk_CommitsChannel1)
```

### 3.2 공통 Fixture

```cpp
class TVControllerFixture : public ::testing::Test {
protected:
    MockTunerForController mockTuner;
    TVController controller{&mockTuner};

    void givenCurrentChannel(const std::string& ch);
    void givenFavorites(std::initializer_list<int> favs);  // FAV_ADD 시퀀스 헬퍼
    void givenSearchList(const std::vector<std::string>& list); // seekCH 스텁
    void whenPress(std::initializer_list<remoteKey> keys);
    void thenExpectSetCH(const std::string& ch, int times = 1);
};
```

### 3.3 경계값 분류 (채널 도메인)

| 경계 클래스 | 값 (문자열) | 출처 |
|-------------|-------------|------|
| 최소 유효 | `"0"` | Tuner `ValidChannels` |
| 중간 | `"4"`, `"5"`, `"12"` | 동일 |
| 최대 유효 | `"99"` | 동일 |
| 최소 무효 (음수) | `"-12"`, `"-2"` | `InvalidChannels` |
| 최대 무효 (상한 초과) | `"100"`, `"9999"` | 동일 |
| 업/다운 래핑 | `0↔99` | README §5 |
| 2자리 커밋 상한 | `"99"` (입력 `9`,`9`) | REQ-CH-02 |
| 선행 0 | `"0"`+`"7"` → `"7"` | REQ-CH-04 |

---

## 4. 테스트 케이스 — 숫자 채널 변경 (REQ-CH)

### 4.1 커밋 트리거 (T-A / T-B)

| TC-ID | REQ | 사전 조건 | 입력 시퀀스 | 기대 `setCH` | 경계/비고 |
|-------|-----|-----------|-------------|--------------|-----------|
| CH-01 | REQ-CH-01 | 초기 0 | `1` → `OK` | `"1"` ×1 | 1자리+확인 |
| CH-02 | REQ-CH-01 | 초기 0 | `9` → `OK` | `"9"` ×1 | 1자리 최대 한 자리 |
| CH-03 | REQ-CH-02 | 초기 0 | `1` → `2` | `"12"` ×1, `OK` 없음 | 2자리 즉시 커밋 |
| CH-04 | REQ-CH-02 | 초기 0 | `9` → `9` | `"99"` ×1 | 2자리 상한 |
| CH-05 | REQ-CH-02 | 초기 0 | `0` → `0` | `"0"` ×1 | `00` → `0` (2자리 규칙) |
| CH-06 | REQ-CH-03 | 초기 0 | `1`,`2`,`3`,`4` | `"12"`, `"34"` 순서 | 2자리마다 자동 커밋 |
| CH-07 | REQ-CH-04 | 초기 0 | `0` → `7` | `"7"` ×1 | 선행 0 (§1.2 #1.5) |
| CH-08 | REQ-CH-04 | 초기 0 | `0` → `0` → `OK` | `"0"` ×1 | `0` 단독 vs `00` |
| CH-09 | REQ-CH-05 | 초기 0 | `4`,`5`,`6` | `"45"`만 | 3번째 `6`은 버퍼 대기 |
| CH-10 | REQ-CH-05 | CH-09 후 | `6` → `OK` | `"6"` ×1 | 대기 `6` 확정 |
| CH-11 | REQ-CH-05 | CH-09 후 | `6` → `8` | `"68"` ×1 | 대기+두 번째 숫자 |
| CH-12 | REQ-CH-05 | CH-09 후 | `CH_UP` | `"46"` 또는 wrap 정책 없이 `"7"` | **기타 키**: `6` 무효, 현재 `45` 유지 후 업 |
| CH-13 | REQ-CH-05 | CH-09 후 | `SEARCH` | `"45"` 유지, 버퍼 클리어 | §6.4 |
| CH-14 | — | 버퍼 비움 | `OK` | 호출 0회 | POL-01 |
| CH-15 | — | `4`만 입력 | (대기) | 호출 0회 | 1자리만으로는 미커밋 |
| CH-16 | — | `1` 입력 후 3초 대기 없음, 즉 `2` | `"12"` | D-05: 3번째 숫자는 새 조합 첫 자리 |

### 4.2 단일 숫자 입력 (키 1회)

| TC-ID | 입력 1회 | 기대 `setCH` | 비고 |
|-------|----------|--------------|------|
| CH-S01 | `KEY_5` | 0회 | 버퍼만 |
| CH-S02 | `KEY_0` | 0회 | 선행 0 시작 |
| CH-S03 | `KEY_1` (버퍼에 `3` 존재) | `"13"` ×1 | 2자리 완성 시 1회 press로 커밋 |

### 4.3 무효 채널 조합 (경계)

| TC-ID | 입력 | 기대 | 비고 |
|-------|------|------|------|
| CH-INV-01 | `1`,`0`,`0` (연속) | `"10"` 후 `"0"` 또는 정책에 따라 `"100"` 시도 → 예외 | `100`은 Tuner 무효 |
| CH-INV-02 | `9`,`9` 후 추가 없음 | `"99"` 성공 | 상한 정상 |
| CH-INV-03 | Mock이 `setCH("100")` throw 설정 | `EXPECT_THROW` 또는 버퍼 초기화 | POL-03 |

---

## 5. 테스트 케이스 — 선호 채널 (REQ-FAV)

### 5.1 선호 추가/삭제 토글 (REQ-FAV-01)

| TC-ID | 사전: 현재 CH | 사전: 선호 목록 | 입력 | 기대 목록 | `setCH` |
|-------|---------------|-----------------|------|-----------|---------|
| FAV-A01 | 6 | {} | `FAV_ADD` | {6} | 0회 |
| FAV-A02 | 6 | {6} | `FAV_ADD` | {} | 0회 (삭제) |
| FAV-A03 | 6 | {1,4,12,56} | `FAV_ADD` | {1,4,6,12,56} | 0회 |
| FAV-A04 | 56 | {1,4,12,56} | `FAV_ADD` | {1,4,12} | 0회 |
| FAV-A05 | 0 | {} | `FAV_ADD` | {0} | 경계: 최소 채널 |
| FAV-A06 | 99 | {} | `FAV_ADD` | {99} | 경계: 최대 채널 |
| FAV-A07 | 6 | — | `4`,`5` 대기 후 `FAV_ADD` | {6}, 버퍼 클리어 | §6.4 |

**경계**: 중복 추가 불가 — 동일 CH 두 번 `FAV_ADD` → 추가 후 삭제(FAV-A02).

### 5.2 다음 선호 채널 — 단일 입력 (REQ-FAV-02)

| TC-ID | favorites | current | 1× `FAV_NEXT` | REQ 매트릭스 |
|-------|-----------|---------|---------------|--------------|
| FAV-N01 | {1,4,12,56} | 6 | `setCH("12")` | F-01 |
| FAV-N02 | {1,4,12,56} | 56 | `setCH("1")` | F-02 wrap |
| FAV-N03 | {1,4,12,56} | 4 | `setCH("12")` | F-03 |
| FAV-N04 | {1,4,12,56} | 12 | `setCH("56")` | §5.1 표 |
| FAV-N05 | {1,4,12,56} | 0 | `setCH("1")` | current < min(fav) |
| FAV-N06 | {7} | 7 | `setCH("7")` | F-04 단일 요소 로테이션 |
| FAV-N07 | {1,4,12,56} | 12 | current **정확히** fav에 있음 | 다음 큰 값 |
| FAV-N08 | {1,4,12,56} | 5 | `setCH("12")` | current가 목록에 없음 |
| FAV-N09 | {1,4,12,56} | 99 | `setCH("1")` | current > max(fav) → wrap |

### 5.3 다음 선호 채널 — **연속 입력** (필수)

알고리즘: `next = min{f ∈ favorites | f > current}`; 없으면 `favorites.front()`.

**사전**: favorites = {1, 4, 12, 56}, 시작 current = **6** (Mock `getCurrentCH` → `"6"`).

| TC-ID | press 횟수 | 누적 입력 | 기대 CH 시퀀스 | 검증 포인트 |
|-------|------------|-----------|----------------|-------------|
| FAV-C01 | 1 | `FAV_NEXT` | 6→**12** | FAV-N01 |
| FAV-C02 | 2 | `FAV_NEXT`×2 | 6→12→**56** | 12 다음은 56 |
| FAV-C03 | 3 | ×3 | 6→12→56→**1** | 56 다음 wrap → 1 |
| FAV-C04 | 4 | ×4 | 6→12→56→1→**4** | 한 사이클 완료 |
| FAV-C05 | 5 | ×5 | …→**12** | FAV-C01과 동일 위상 |
| FAV-C06 | 10 | ×10 | 2주기 완료 | 호출 10회, Mock `InSequence` |

**다른 시작점 (연속)**

| TC-ID | favorites | start | ×3 `FAV_NEXT` 기대 시퀀스 |
|-------|-----------|-------|---------------------------|
| FAV-C10 | {1,4,12,56} | 56 | 56→1→4 |
| FAV-C11 | {1,4,12,56} | 1 | 1→4→12 |
| FAV-C12 | {7} | 7 | 7→7→7 (단일) |
| FAV-C13 | {0,50,99} | 25 | 50→99→0 (경계값 fav 집합) |

**Mock 검증 예시 (FAV-C02)**:

```cpp
InSequence seq;
EXPECT_CALL(mockTuner, getCurrentCH()).WillRepeatedly(...); // 또는 press마다 Return 갱신
EXPECT_CALL(mockTuner, setCH("12"));
EXPECT_CALL(mockTuner, setCH("56"));
controller.pushButton(KEY_FAV_NEXT);
controller.pushButton(KEY_FAV_NEXT);
```

> **주의**: `setCH` 후 `getCurrentCH`가 갱신된다고 가정할 경우, press마다 Return을 `"12"`, `"56"`으로 바꾸는 **상태ful 스텁**이 필요하다.

### 5.4 선호 목록 **없음** (필수)

| TC-ID | favorites | current | 입력 | 기대 `setCH` | 기대 CH |
|-------|-----------|---------|------|--------------|---------|
| FAV-E01 | {} | 6 | `FAV_NEXT` ×1 | 0회 | 6 유지 | POL-02 |
| FAV-E02 | {} | 0 | `FAV_NEXT` ×3 | 0회 | 0 유지 | 연속 무효 |
| FAV-E03 | {} | 99 | `FAV_ADD` 후 ×1 `FAV_NEXT` | 1회만 (`setCH` on NEXT) | 99→99 또는 99→? | 단일 fav 로테이션 |
| FAV-E04 | {} | 6 | `FAV_ADD` → `FAV_NEXT` | add 후 {6}, next → 6 | 빈 목록에서 복구 |

---

## 6. 테스트 케이스 — 채널 검색 (REQ-SEEK)

| TC-ID | Mock `seekCH` 동작 | 입력 | 기대 |
|-------|-------------------|------|------|
| SEEK-01 | `"4"`,`"6"`,`"14"`, `""` | `SEARCH` ×1 | `seekCH` 4회, 내부 목록 [4,6,14] |
| SEEK-02 | 항상 `""` | `SEARCH` | 1회 호출 후 빈 목록 |
| SEEK-03 | `TunerTest::testSeekCh10times` 패턴 | `SEARCH` | 10회 성공, 각 CH 0~99 |
| SEEK-04 | 반복 반환 | `SEARCH` ×2 | 목록 덮어쓰기 vs 누적 — **README는 “저장”** → 2회째 전체 재검색으로 정의 후 고정 |
| SEEK-05 | — | `4`,`5` 대기 + `SEARCH` | 버퍼 클리어 후 검색 | §6.4 |

---

## 7. 테스트 케이스 — 채널 업/다운 (REQ-UD)

### 7.1 검색 목록 **없음** (REQ-UD-01)

| TC-ID | current | key | expected | 경계 |
|-------|---------|-----|----------|------|
| UD-01 | 6 | UP | 7 | — |
| UD-02 | 6 | DOWN | 5 | — |
| UD-03 | 99 | UP | 0 | 상한 wrap |
| UD-04 | 0 | DOWN | 99 | 하한 wrap |
| UD-05 | 0 | UP | 1 | — |
| UD-06 | 99 | DOWN | 98 | — |

**파라미터화 (권장)**:

```cpp
INSTANTIATE_TEST_SUITE_P(ChannelWrap, TVControllerUpDownWithoutSearchTest,
    Values(std::tuple{"99", KEY_CH_UP, "0"}, ...));
```

### 7.2 검색 목록 **있음** (REQ-UD-02)

사전: SEARCH로 목록 = {4, 6, 14}.

| TC-ID | current | key | expected | 설명 |
|-------|---------|-----|----------|------|
| UD-S01 | 6 | UP | 14 | 목록 내 다음 |
| UD-S02 | 6 | DOWN | 4 | 목록 내 이전 |
| UD-S03 | 15 | UP | 4 | wrap → 처음 |
| UD-S04 | 15 | DOWN | 14 | wrap → 끝 |
| UD-S05 | 4 | DOWN | 14 | 최소에서 down → 끝 |
| UD-S06 | 14 | UP | 4 | 최대에서 up → 처음 |
| UD-S07 | 6 | UP×3 | 14→4→6 | 연속 업 (목록 순환) |

### 7.3 검색 목록 vs 일반 업/다운 전환

| TC-ID | 시퀀스 | 기대 |
|-------|--------|------|
| UD-T01 | SEARCH → UP from 6 | 14 (목록 모드) |
| UD-T02 | (목록 없음) UP from 6 → SEARCH → UP | 7 → … → 14 |

---

## 8. 테스트 케이스 — 기타 버튼·상호작용 (필수)

### 8.1 키 1회당 부수 효과 (§2)

| TC-ID | 키 | 사전 | `setCH` | `seekCH` | 내부 상태 |
|-------|-----|------|---------|----------|-----------|
| BTN-01 | `CH_UP` | CH=6, no search | 1회 `"7"` | 0 | — |
| BTN-02 | `SEARCH` | — | 0 | ≥1 | searchedChannels |
| BTN-03 | `FAV_ADD` | CH=3 | 0 | 0 | favorites |
| BTN-04 | `FAV_NEXT` | fav non-empty | 1 | 0 | — |

### 8.2 숫자 버퍼 vs 기타 키 (§6.4)

| TC-ID | 시퀀스 | 기대 |
|-------|--------|------|
| BTN-X01 | `4`,`5`,`6` → `CH_DOWN` | CH=45, `setCH("44")` 또는 목록/일반 정책에 따름; **버퍼 6 폐기** |
| BTN-X02 | `4`,`5`,`6` → `FAV_NEXT` | 45 유지, 6 무효 |
| BTN-X03 | `4`,`5`,`6` → `FAV_ADD` | 45 유지, fav 토글은 45 기준 |
| BTN-X04 | `3` 대기 → `CH_UP` | 버퍼 클리어 후 업만 |
| BTN-X05 | `3` 대기 → `OK` | `setCH("3")` |

### 8.3 복합 시나리오 (E2E 스타일)

| TC-ID | 시나리오 | 검증 |
|-------|----------|------|
| E2E-01 | `1`,`2` → FAV_ADD → FAV_NEXT×2 | 12 저장, next 동작 |
| E2E-02 | SEARCH → UD-S01~S04 일부 | 검색+업다운 연동 |
| E2E-03 | fav 구축 → CH 50 수동 → FAV_NEXT 연속 | 수동 CH와 fav 알고리즘 |
| E2E-04 | `0`,`7` → UP → DOWN | 7→8→7 (no search) |

---

## 9. 요구사항 추적 매트릭스 (RTM)

| REQ-ID | 테스트 ID (대표) | 우선순위 |
|--------|------------------|----------|
| REQ-CH-01 | CH-01, CH-02 | P0 |
| REQ-CH-02 | CH-03, CH-04, CH-05 | P0 |
| REQ-CH-03 | CH-06 | P0 |
| REQ-CH-04 | CH-07, CH-08 | P0 |
| REQ-CH-05 | CH-09~CH-13, CH-S*, BTN-X* | P1 |
| REQ-FAV-01 | FAV-A01~A07 | P1 |
| REQ-FAV-02 | FAV-N*, **FAV-C***, FAV-E* | P0 (연속/빈 목록) |
| REQ-SEEK-01 | SEEK-01~05 | P1 |
| REQ-UD-01 | UD-01~06 | P1 |
| REQ-UD-02 | UD-S*, UD-T* | P1 |
| §0.1 Tuner 계약 | CH-INV*, (TunerTest 참조) | P2 |

---

## 10. 구현 로드맵 (TDD)

| 단계 | 테스트 묶음 | 완료 기준 |
|------|---------------|-----------|
| 1 | CH-01~04, CH-14~15 | 숫자+OK, 2자리 커밋 RED→GREEN |
| 2 | CH-06~08, CH-09~11 | 연속 숫자·선행 0 |
| 3 | FAV-A*, FAV-N*, **FAV-C***, **FAV-E*** | 선호 전체 |
| 4 | UD-01~06, UD-S*, UD-T* | 업/다운 |
| 5 | SEEK-*, BTN-X*, E2E-* | 검색·충돌·복합 |
| 6 | CH-INV*, 커버리지 | 예외·90%+ line |

---

## 11. 결함 심각도·출구 기준

### 11.1 심각도

| 등급 | 정의 | 예 |
|------|------|-----|
| S1 | 잘못된 채널로 시청 전환 | wrap 누락, 잘못된 `setCH` |
| S2 | 요구 미구현 | FAV_NEXT 연속 시 두 번째 press 무동작 |
| S3 | 부수 상태만 오류 | fav 목록 중복 (CH는 정상) |
| S4 | 정책 문서와 불일치 | 빈 fav에서 throw vs no-op |

### 11.2 출구 기준 (Release)

- [ ] P0·P1 TC 100% Pass  
- [ ] `ctest` 전체 Pass (기존 `TunerTest` 포함)  
- [ ] REQ-ID RTM 100% 매핑  
- [ ] FAV-C*, FAV-E* 명시 통과  
- [ ] (선택) `TVController` line coverage ≥ 90%  
- [ ] POL-01~04 구현·문서 일치 확인  

---

## 12. 부록 — `remoteKey` 확장 체크리스트

구현 시 enum에 아래 키가 있어야 해당 TC 실행 가능하다.

- [ ] `KEY_0` … `KEY_9`  
- [ ] `KEY_OK`  
- [ ] `KEY_CH_UP`, `KEY_CH_DOWN`  
- [ ] `KEY_SEARCH`  
- [ ] `KEY_FAV_ADD`, `KEY_FAV_NEXT`  

현재 코드베이스(`remoteKey.h`)는 `KEY_1`, `KEY_OK`만 존재 → **테스트는 확장 enum 기준으로 작성**하고, 키 추가 커밋과 함께 TC를 활성화한다.

---

*문서 버전: 1.0 — requirements_analysis.md v1.0 및 README 기준*
