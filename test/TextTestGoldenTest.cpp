#include <gtest/gtest.h>
#include "TVController.h"
#include "remoteKey.h"
#include "support/CoutCapture.hpp"
#include "support/FakeTunerForText.hpp"
#include "support/GoldenMaster.hpp"
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

class TextTestFixture : public ::testing::Test {
protected:
    FakeTunerForText tuner_;
    TVController controller_{tuner_};
    std::ostringstream transcript_;

    void setCurrentChannel(const std::string& ch) { tuner_.setCH(ch); }

    void setSeekResults(std::initializer_list<std::string> channels) {
        tuner_.setSeekQueue(channels);
    }

    void press(std::initializer_list<remoteKey> keys) {
        for (remoteKey key : keys) {
            transcript_ << "> " << to_string(key) << '\n';
            CoutCapture capture;
            controller_.pushButton(key);
            transcript_ << capture.str();
        }
    }

    void assertGolden() {
        const ::testing::TestInfo* const info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        golden::assertMatchesGolden(info->test_suite_name(), info->name(),
                                    transcript_.str());
    }
};

// README §1 — 숫자 버튼 채널 변경
TEST_F(TextTestFixture, README_digit_1_ok) {
    press({remoteKey::KEY_1, remoteKey::KEY_OK});
    assertGolden();
}

TEST_F(TextTestFixture, README_digit_1_2) {
    press({remoteKey::KEY_1, remoteKey::KEY_2});
    assertGolden();
}

TEST_F(TextTestFixture, README_digit_1_2_3_4) {
    press({remoteKey::KEY_1, remoteKey::KEY_2, remoteKey::KEY_3, remoteKey::KEY_4});
    assertGolden();
}

TEST_F(TextTestFixture, README_digit_4_5_6_then_ok) {
    press({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6, remoteKey::KEY_OK});
    assertGolden();
}

TEST_F(TextTestFixture, README_digit_4_5_6_then_ch_up) {
    press({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6, remoteKey::KEY_CH_UP});
    assertGolden();
}

TEST_F(TextTestFixture, README_digit_0_7) {
    press({remoteKey::KEY_0, remoteKey::KEY_7});
    assertGolden();
}

// README §3 — 다음 선호 채널
TEST_F(TextTestFixture, README_fav_next_from_6) {
    setCurrentChannel("1");
    press({remoteKey::KEY_FAV_ADD});
    setCurrentChannel("4");
    press({remoteKey::KEY_FAV_ADD});
    setCurrentChannel("12");
    press({remoteKey::KEY_FAV_ADD});
    setCurrentChannel("56");
    press({remoteKey::KEY_FAV_ADD});
    setCurrentChannel("6");
    press({remoteKey::KEY_FAV_NEXT});
    assertGolden();
}

TEST_F(TextTestFixture, README_fav_next_wrap_from_56) {
    setCurrentChannel("1");
    press({remoteKey::KEY_FAV_ADD});
    setCurrentChannel("56");
    press({remoteKey::KEY_FAV_ADD});
    setCurrentChannel("56");
    press({remoteKey::KEY_FAV_NEXT});
    assertGolden();
}

// README §5 — 업/다운 (검색 없음)
TEST_F(TextTestFixture, README_ch_up_down_without_search) {
    setCurrentChannel("6");
    press({remoteKey::KEY_CH_UP});
    press({remoteKey::KEY_CH_DOWN});
    assertGolden();
}

TEST_F(TextTestFixture, README_wrap_99_up_and_0_down) {
    setCurrentChannel("99");
    press({remoteKey::KEY_CH_UP});
    setCurrentChannel("0");
    press({remoteKey::KEY_CH_DOWN});
    assertGolden();
}

// README §6 — 업/다운 (검색 있음)
TEST_F(TextTestFixture, README_ch_up_down_with_search) {
    setCurrentChannel("6");
    setSeekResults({"4", "6", "14"});
    press({remoteKey::KEY_SEARCH});
    press({remoteKey::KEY_CH_UP});
    press({remoteKey::KEY_CH_DOWN});
    assertGolden();
}

TEST_F(TextTestFixture, README_ch_up_down_wrap_with_search) {
    setCurrentChannel("15");
    setSeekResults({"4", "6", "14"});
    press({remoteKey::KEY_SEARCH});
    press({remoteKey::KEY_CH_UP});
    press({remoteKey::KEY_CH_DOWN});
    assertGolden();
}
