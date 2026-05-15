#include <gtest/gtest.h>
#include "TVController.h"
#include "FakeTuner.h"
#include <memory>
#include "remoteKey.h"

// ── 공통 픽스처 (SetUp/TearDown 자동 호출) ─────────────────
class ControllerTest : public ::testing::Test {
protected:
    std::unique_ptr<FakeTuner> tuner;
    std::unique_ptr<TVController> ctrl;

    void SetUp() override {
        tuner = std::make_unique<FakeTuner>(std::vector<int>{1, 4, 12, 56});
        ctrl = std::make_unique<TVController>(tuner.get());
    }
};

// ── 기능 1: 숫자 버튼 채널 변경 ─────────────────
// S1-1: 한 자리 입력 + 확인
TEST_F(ControllerTest, PressNumber1ThenConfirm) {    
    ctrl->pushButton(remoteKey::KEY_1);      // Given
    ctrl->pushButton(remoteKey::KEY_OK);      // When
    EXPECT_EQ("1", tuner->getCurrentCH()); // Then
}

// S1-2: 두 자리 자동 변경
TEST_F(ControllerTest, Press1Then2_AutoChange) {
    ctrl->pushButton(remoteKey::KEY_1);
    ctrl->pushButton(remoteKey::KEY_2);      // 두 자리 완성 → 자동
    EXPECT_EQ("12", tuner->getCurrentCH());
}

// S1-3: 연속 두 자리 × 2회
TEST_F(ControllerTest, Press1234_TwoStageChange) {
    ctrl->pushButton(remoteKey::KEY_1); ctrl->pushButton(remoteKey::KEY_2); // -> 12
    ctrl->pushButton(remoteKey::KEY_3); ctrl->pushButton(remoteKey::KEY_4); // -> 34
    EXPECT_EQ("34", tuner->getCurrentCH());
}

// S1-4: 버퍼 무효화
TEST_F(ControllerTest, OtherButtonCancelsBuffer) {
    ctrl->pushButton(remoteKey::KEY_4);
    ctrl->pushButton(remoteKey::KEY_5);
    ctrl->pushButton(remoteKey::KEY_6); // 3자리 -> 무효화
    ctrl->pushButton(remoteKey::KEY_OTHER);
    // 6은 무효화 → 채널 변화 없음
    EXPECT_EQ("0", tuner->getCurrentCH());
}

// S1-5: '0','7' -> 7번
TEST_F(ControllerTest, Zero7_SingleDigit7) {
    ctrl->pushButton(remoteKey::KEY_0);
    ctrl->pushButton(remoteKey::KEY_7);
    EXPECT_EQ("7", tuner->getCurrentCH());
}

TEST_F(ControllerTest, FavoriteAdd_NewChannel) {
    tuner->setCH("12");
    ctrl->pushButton(remoteKey::KEY_FAVORITE);
    const auto& favs = ctrl->getFavoriteChannels();
    EXPECT_NE(favs.end(),
              std::find(favs.begin(), favs.end(), 12));
}

TEST_F(ControllerTest, FavoriteToggle_Remove) {
    tuner->setCH("12");
    ctrl->pushButton(remoteKey::KEY_FAVORITE);
    ctrl->pushButton(remoteKey::KEY_FAVORITE);
    const auto& favs = ctrl->getFavoriteChannels();
    EXPECT_EQ(favs.end(),
              std::find(favs.begin(), favs.end(), 12));
}

// S2-3: 토글 시나리오 전체
TEST_F(ControllerTest, FavoriteToggleScenario) {
    for (int ch : {12, 8, 37, 8, 6}) {
        tuner->setCH(std::to_string(ch));
        ctrl->pushButton(remoteKey::KEY_FAVORITE);
    }

    const auto& favs = ctrl->getFavoriteChannels();
    // {6, 12, 37} 만 남아야 함
    EXPECT_EQ(3u, favs.size());
    EXPECT_NE(favs.end(),
              std::find(favs.begin(), favs.end(), 6));
    EXPECT_NE(favs.end(),
              std::find(favs.begin(), favs.end(), 12));
    EXPECT_NE(favs.end(),
              std::find(favs.begin(), favs.end(), 37));
}

// —— 기능 3: 다음 선호 채널 —————————————————————————————
TEST_F(ControllerTest, NextFavorite_Normal) {
    for (int ch : {1, 4, 12, 56}) ctrl->addFavorite(ch);
    tuner->setCH("6");
    ctrl->pushButton(remoteKey::KEY_NEXT_FAVORITE);
    EXPECT_EQ("12", tuner->getCurrentCH());
}

TEST_F(ControllerTest, NextFavorite_WrapAround) {
    ctrl->addFavorite(1); ctrl->addFavorite(56);
    tuner->setCH("56");
    ctrl->pushButton(remoteKey::KEY_NEXT_FAVORITE);
    EXPECT_EQ("1", tuner->getCurrentCH());
}

TEST_F(ControllerTest, NextFavorite_EmptyList) {
    tuner->setCH("6");
    ctrl->pushButton(remoteKey::KEY_NEXT_FAVORITE);
    EXPECT_EQ("6", tuner->getCurrentCH()); // 변화 없음
}