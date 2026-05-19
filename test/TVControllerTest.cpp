#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Tuner.h"
#include "TVController.h"
#include "remoteKey.h"
#include <deque>
#include <initializer_list>
#include <string>
#include <vector>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::Throw;

class MockTunerForController : public Tuner {
public:
    MOCK_METHOD(std::string, seekCH, (), (override));
    MOCK_METHOD(void, setCH, (const std::string& ch), (override));
    MOCK_METHOD(std::string, getCurrentCH, (), (override));
};

class TVControllerFixture : public ::testing::Test {
protected:
    MockTunerForController mockTuner;
    TVController controller{mockTuner};
    std::string currentChannel_{"0"};

    void SetUp() override {
        wireStatefulTuner();
    }

    void wireStatefulTuner() {
        ON_CALL(mockTuner, getCurrentCH())
            .WillByDefault([this] { return currentChannel_; });
        ON_CALL(mockTuner, setCH(_))
            .WillByDefault([this](const std::string& ch) { currentChannel_ = ch; });
    }

    void givenCurrentChannel(const std::string& ch) {
        currentChannel_ = ch;
    }

    void expectStatefulCurrentChannel() {
        EXPECT_CALL(mockTuner, getCurrentCH())
            .WillRepeatedly([this] { return currentChannel_; });
    }

    void whenPress(std::initializer_list<remoteKey> keys) {
        for (remoteKey key : keys) {
            controller.pushButton(key);
        }
    }

    void givenFavoritesByAdd(std::initializer_list<int> favs) {
        for (int fav : favs) {
            givenCurrentChannel(std::to_string(fav));
            controller.pushButton(remoteKey::KEY_FAV_ADD);
        }
    }

    void givenSearchList(const std::vector<std::string>& list) {
        std::deque<std::string> returns(list.begin(), list.end());
        returns.push_back("");
        EXPECT_CALL(mockTuner, seekCH())
            .Times(static_cast<int>(returns.size()))
            .WillRepeatedly([&returns]() {
                if (returns.empty()) {
                    return std::string{};
                }
                std::string ch = returns.front();
                returns.pop_front();
                return ch;
            });
        controller.pushButton(remoteKey::KEY_SEARCH);
        testing::Mock::VerifyAndClearExpectations(&mockTuner);
        wireStatefulTuner();
    }
};

// --- REQ-CH: numeric channel change ---

TEST_F(TVControllerFixture, REQ_CH_01_OneDigitThenOk_CommitsChannel1) {
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    whenPress({remoteKey::KEY_1, remoteKey::KEY_OK});
}

TEST_F(TVControllerFixture, REQ_CH_01_OneDigit9ThenOk_CommitsChannel9) {
    EXPECT_CALL(mockTuner, setCH("9")).Times(1);
    whenPress({remoteKey::KEY_9, remoteKey::KEY_OK});
}

TEST_F(TVControllerFixture, REQ_CH_02_TwoDigits_CommitsWithoutOk) {
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    whenPress({remoteKey::KEY_1, remoteKey::KEY_2});
}

TEST_F(TVControllerFixture, REQ_CH_02_TwoDigits99_CommitsUpperBound) {
    EXPECT_CALL(mockTuner, setCH("99")).Times(1);
    whenPress({remoteKey::KEY_9, remoteKey::KEY_9});
}

TEST_F(TVControllerFixture, REQ_CH_02_DoubleZero_CommitsChannel0) {
    EXPECT_CALL(mockTuner, setCH("0")).Times(1);
    whenPress({remoteKey::KEY_0, remoteKey::KEY_0});
}

TEST_F(TVControllerFixture, REQ_CH_03_FourDigits_AutoCommitTwice) {
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("12"));
    EXPECT_CALL(mockTuner, setCH("34"));
    whenPress({remoteKey::KEY_1, remoteKey::KEY_2, remoteKey::KEY_3, remoteKey::KEY_4});
}

TEST_F(TVControllerFixture, REQ_CH_04_LeadingZero_Commits7) {
    EXPECT_CALL(mockTuner, setCH("7")).Times(1);
    whenPress({remoteKey::KEY_0, remoteKey::KEY_7});
}

TEST_F(TVControllerFixture, REQ_CH_04_ZeroThenZeroThenOk_Commits0) {
    EXPECT_CALL(mockTuner, setCH("0")).Times(1);
    whenPress({remoteKey::KEY_0, remoteKey::KEY_0, remoteKey::KEY_OK});
}

TEST_F(TVControllerFixture, REQ_CH_05_ThirdDigitWaits) {
    EXPECT_CALL(mockTuner, setCH("45")).Times(1);
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6});
}

TEST_F(TVControllerFixture, REQ_CH_05_PendingSixThenOk_Commits6) {
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6});
    EXPECT_CALL(mockTuner, setCH("6")).Times(1);
    controller.pushButton(remoteKey::KEY_OK);
}

TEST_F(TVControllerFixture, REQ_CH_05_PendingSixThenEight_Commits68) {
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6});
    EXPECT_CALL(mockTuner, setCH("68")).Times(1);
    controller.pushButton(remoteKey::KEY_8);
}

TEST_F(TVControllerFixture, REQ_CH_05_PendingSixThenChUp_From45To46) {
    givenCurrentChannel("45");
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("46")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);
}

TEST_F(TVControllerFixture, REQ_CH_05_PendingSixThenSearch_NoExtraSetCH) {
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6});
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(Return(""));
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_SEARCH);
}

TEST_F(TVControllerFixture, POL_01_EmptyBufferOk_NoSetCH) {
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_OK);
}

TEST_F(TVControllerFixture, REQ_CH_15_SingleDigitNoCommit_UntilOk) {
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_4);
}

TEST_F(TVControllerFixture, REQ_CH_S01_SingleDigit5_NoSetCH) {
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_5);
}

TEST_F(TVControllerFixture, REQ_CH_S02_SingleDigit0_NoSetCH) {
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_0);
}

TEST_F(TVControllerFixture, REQ_CH_S03_Buffer1Then3_Commits13) {
    controller.pushButton(remoteKey::KEY_1);
    EXPECT_CALL(mockTuner, setCH("13")).Times(1);
    controller.pushButton(remoteKey::KEY_3);
}

TEST_F(TVControllerFixture, REQ_CH_INV_01_OneZeroZero_Commits10ThenWaits) {
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("10"));
    whenPress({remoteKey::KEY_1, remoteKey::KEY_0, remoteKey::KEY_0});
}

TEST_F(TVControllerFixture, REQ_CH_INV_03_TunerThrow_PropagatesException) {
    EXPECT_CALL(mockTuner, setCH("12"))
        .WillOnce(Throw(std::invalid_argument("Invalid channel")));
    EXPECT_THROW(whenPress({remoteKey::KEY_1, remoteKey::KEY_2}), std::invalid_argument);
}

// --- REQ-FAV: favorites ---

TEST_F(TVControllerFixture, REQ_FAV_A01_AddFavorite_NoSetCH) {
    givenCurrentChannel("6");
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAV_ADD);
}

TEST_F(TVControllerFixture, REQ_FAV_A02_ToggleOffFavorite_NoSetCH) {
    givenCurrentChannel("6");
    controller.pushButton(remoteKey::KEY_FAV_ADD);
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAV_ADD);
}

TEST_F(TVControllerFixture, REQ_FAV_A02_ToggleOff_ThenFavNextNoOp) {
    givenCurrentChannel("6");
    controller.pushButton(remoteKey::KEY_FAV_ADD);
    controller.pushButton(remoteKey::KEY_FAV_ADD);
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_A07_PendingDigitsThenFavAdd_ClearsBuffer) {
    givenCurrentChannel("6");
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5});
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAV_ADD);
}

TEST_F(TVControllerFixture, REQ_FAV_N01_From6To12) {
    givenCurrentChannel("6");
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("6");
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_N02_From56WrapsTo1) {
    givenCurrentChannel("6");
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("56");
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_N03_From4To12) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("4");
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_N04_From12To56) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("12");
    EXPECT_CALL(mockTuner, setCH("56")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_N05_From0To1) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("0");
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_N06_SingleFavoriteRotates) {
    givenCurrentChannel("7");
    controller.pushButton(remoteKey::KEY_FAV_ADD);
    EXPECT_CALL(mockTuner, setCH("7")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_N08_CurrentNotInList_GoesTo12) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("5");
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_N09_CurrentAboveMax_WrapsTo1) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("99");
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_C01_ContinuousNext_6To12) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("6");
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_C02_ContinuousNext_6To12To56) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("6");
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("12"));
    EXPECT_CALL(mockTuner, setCH("56"));
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_C03_ContinuousNext_ThreePresses) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("6");
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("12"));
    EXPECT_CALL(mockTuner, setCH("56"));
    EXPECT_CALL(mockTuner, setCH("1"));
    whenPress({remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT});
}

TEST_F(TVControllerFixture, REQ_FAV_C04_ContinuousNext_FourPresses) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("6");
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("12"));
    EXPECT_CALL(mockTuner, setCH("56"));
    EXPECT_CALL(mockTuner, setCH("1"));
    EXPECT_CALL(mockTuner, setCH("4"));
    whenPress({remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT,
               remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT});
}

TEST_F(TVControllerFixture, REQ_FAV_C10_From56_ThreePresses) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("56");
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("1"));
    EXPECT_CALL(mockTuner, setCH("4"));
    EXPECT_CALL(mockTuner, setCH("12"));
    whenPress({remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT});
}

TEST_F(TVControllerFixture, REQ_FAV_C11_From1_ThreePresses) {
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("1");
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("4"));
    EXPECT_CALL(mockTuner, setCH("12"));
    EXPECT_CALL(mockTuner, setCH("56"));
    whenPress({remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT});
}

TEST_F(TVControllerFixture, REQ_FAV_C12_SingleFavorite_ThreePresses) {
    givenCurrentChannel("7");
    controller.pushButton(remoteKey::KEY_FAV_ADD);
    EXPECT_CALL(mockTuner, setCH("7")).Times(3);
    whenPress({remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT});
}

TEST_F(TVControllerFixture, REQ_FAV_C13_BoundaryFavorites_From25) {
    givenFavoritesByAdd({0, 50, 99});
    givenCurrentChannel("25");
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("50"));
    EXPECT_CALL(mockTuner, setCH("99"));
    EXPECT_CALL(mockTuner, setCH("0"));
    whenPress({remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT});
}

TEST_F(TVControllerFixture, POL_02_EmptyFavorites_NoSetCH) {
    givenCurrentChannel("6");
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_FAV_E02_EmptyFavorites_ContinuousNoOp) {
    givenCurrentChannel("0");
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    whenPress({remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT, remoteKey::KEY_FAV_NEXT});
}

TEST_F(TVControllerFixture, REQ_FAV_E04_AddThenNext_FromEmpty) {
    givenCurrentChannel("6");
    controller.pushButton(remoteKey::KEY_FAV_ADD);
    EXPECT_CALL(mockTuner, setCH("6")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

// --- REQ-SEEK: channel search ---

TEST_F(TVControllerFixture, REQ_SEEK_01_BuildsSearchList) {
    EXPECT_CALL(mockTuner, seekCH())
        .WillOnce(Return("4"))
        .WillOnce(Return("6"))
        .WillOnce(Return("14"))
        .WillOnce(Return(""));
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_SEARCH);
}

TEST_F(TVControllerFixture, REQ_SEEK_02_EmptySearchResult) {
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(Return(""));
    controller.pushButton(remoteKey::KEY_SEARCH);
}

TEST_F(TVControllerFixture, REQ_SEEK_05_PendingDigitsThenSearch_ClearsBuffer) {
    controller.pushButton(remoteKey::KEY_4);
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(Return(""));
    EXPECT_CALL(mockTuner, setCH(_)).Times(0);
    controller.pushButton(remoteKey::KEY_SEARCH);
}

// --- REQ-UD: channel up/down without search ---

class TVControllerUpDownWithoutSearchTest
    : public TVControllerFixture,
      public ::testing::WithParamInterface<std::tuple<std::string, remoteKey, std::string>> {};

TEST_P(TVControllerUpDownWithoutSearchTest, ChannelWrap) {
    auto [current, key, expected] = GetParam();
    givenCurrentChannel(current);
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH(expected)).Times(1);
    controller.pushButton(key);
}

INSTANTIATE_TEST_SUITE_P(
    ChannelWrap,
    TVControllerUpDownWithoutSearchTest,
    ::testing::Values(
        std::make_tuple("6", remoteKey::KEY_CH_UP, "7"),
        std::make_tuple("6", remoteKey::KEY_CH_DOWN, "5"),
        std::make_tuple("99", remoteKey::KEY_CH_UP, "0"),
        std::make_tuple("0", remoteKey::KEY_CH_DOWN, "99"),
        std::make_tuple("0", remoteKey::KEY_CH_UP, "1"),
        std::make_tuple("99", remoteKey::KEY_CH_DOWN, "98")));

// --- REQ-UD: channel up/down with search list ---

TEST_F(TVControllerFixture, REQ_UD_S01_WithSearch_UpFrom6To14) {
    givenCurrentChannel("6");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("14")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);
}

TEST_F(TVControllerFixture, REQ_UD_S02_WithSearch_DownFrom6To4) {
    givenCurrentChannel("6");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("4")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);
}

TEST_F(TVControllerFixture, REQ_UD_S03_WithSearch_UpFrom15WrapsTo4) {
    givenCurrentChannel("15");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("4")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);
}

TEST_F(TVControllerFixture, REQ_UD_S04_WithSearch_DownFrom15WrapsTo14) {
    givenCurrentChannel("15");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("14")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);
}

TEST_F(TVControllerFixture, REQ_UD_S05_WithSearch_DownFrom4WrapsTo14) {
    givenCurrentChannel("4");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("14")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);
}

TEST_F(TVControllerFixture, REQ_UD_S06_WithSearch_UpFrom14WrapsTo4) {
    givenCurrentChannel("14");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("4")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);
}

TEST_F(TVControllerFixture, REQ_UD_S07_WithSearch_ThreeUps) {
    givenCurrentChannel("6");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("14"));
    EXPECT_CALL(mockTuner, setCH("4"));
    EXPECT_CALL(mockTuner, setCH("6"));
    whenPress({remoteKey::KEY_CH_UP, remoteKey::KEY_CH_UP, remoteKey::KEY_CH_UP});
}

TEST_F(TVControllerFixture, REQ_UD_T01_SearchThenUp_UsesList) {
    givenCurrentChannel("6");
    givenSearchList({"4", "6", "14"});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("14")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);
}

// --- Button interactions ---

TEST_F(TVControllerFixture, REQ_BTN_01_ChUp_NoSearch) {
    givenCurrentChannel("6");
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("7")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);
}

TEST_F(TVControllerFixture, REQ_BTN_02_Search_CallsSeekCH) {
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(Return(""));
    controller.pushButton(remoteKey::KEY_SEARCH);
}

TEST_F(TVControllerFixture, REQ_BTN_X01_Pending456ThenDown) {
    givenCurrentChannel("45");
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6});
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("44")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);
}

TEST_F(TVControllerFixture, REQ_BTN_X02_Pending456ThenFavNext) {
    givenCurrentChannel("45");
    givenFavoritesByAdd({1, 4, 12, 56});
    givenCurrentChannel("45");
    whenPress({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6});
    EXPECT_CALL(mockTuner, setCH("56")).Times(1);
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_BTN_X04_Pending3ThenUp) {
    givenCurrentChannel("3");
    controller.pushButton(remoteKey::KEY_3);
    expectStatefulCurrentChannel();
    EXPECT_CALL(mockTuner, setCH("4")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);
}

TEST_F(TVControllerFixture, REQ_BTN_X05_Pending3ThenOk) {
    controller.pushButton(remoteKey::KEY_3);
    EXPECT_CALL(mockTuner, setCH("3")).Times(1);
    controller.pushButton(remoteKey::KEY_OK);
}

// --- E2E scenarios ---

TEST_F(TVControllerFixture, REQ_E2E_01_Channel12FavNext) {
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("12"));
    whenPress({remoteKey::KEY_1, remoteKey::KEY_2});
    controller.pushButton(remoteKey::KEY_FAV_ADD);
    givenCurrentChannel("12");
    EXPECT_CALL(mockTuner, setCH("12"));
    controller.pushButton(remoteKey::KEY_FAV_NEXT);
}

TEST_F(TVControllerFixture, REQ_E2E_04_ZeroSevenUpDown) {
    givenCurrentChannel("0");
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("7"));
    EXPECT_CALL(mockTuner, setCH("8"));
    EXPECT_CALL(mockTuner, setCH("7"));
    whenPress({remoteKey::KEY_0, remoteKey::KEY_7, remoteKey::KEY_CH_UP, remoteKey::KEY_CH_DOWN});
}
