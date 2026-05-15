#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Tuner.h"
#include <vector>
#include <string>

class MockTuner : public Tuner {
public:
    MOCK_METHOD(std::string, seekCH, (), (override));
    MOCK_METHOD(void, setCH, (const std::string& ch), (override));
    MOCK_METHOD(std::string, getCurrentCH, (), (override));
};

class TunerTest : public ::testing::Test {
protected:
    MockTuner tuner;
};

// 초기 채널값 확인 테스트
TEST_F(TunerTest, initChannel) {
    EXPECT_CALL(tuner, getCurrentCH()).WillOnce(::testing::Return("0"));
    int initCh = std::stoi(tuner.getCurrentCH());
    EXPECT_TRUE(initCh >= 0 && initCh <= 99);
}

// 채널설정테스트 : 유효한 채널인 경우
class TunerValidChannelTest : public ::testing::TestWithParam<std::string> {
protected:
    MockTuner tuner;
};

TEST_P(TunerValidChannelTest, testSetChForValidChannel) {
    std::string channel = GetParam();
    EXPECT_CALL(tuner, setCH(channel));
    EXPECT_CALL(tuner, getCurrentCH()).WillOnce(::testing::Return(channel));
    tuner.setCH(channel);
    EXPECT_EQ(channel, tuner.getCurrentCH());
}

INSTANTIATE_TEST_SUITE_P(ValidChannels, TunerValidChannelTest,
    ::testing::Values("0", "4", "5", "12", "99"));

// 채널설정테스트 : 유효하지 않은 채널인 경우
class TunerInvalidChannelTest : public ::testing::TestWithParam<std::string> {
protected:
    MockTuner tuner;
};

TEST_P(TunerInvalidChannelTest, testSetChForInvalidChannel) {
    std::string channel = GetParam();
    EXPECT_CALL(tuner, setCH(channel))
        .WillOnce(::testing::Throw(std::invalid_argument("Invalid channel")));
    EXPECT_THROW(tuner.setCH(channel), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(InvalidChannels, TunerInvalidChannelTest,
    ::testing::Values("-12", "-2", "-0", "100", "9999"));

// 채널검색테스트 : 시작 채널로부터 10개 채널 검색 테스트
TEST_F(TunerTest, testSeekCh10times) {
    EXPECT_CALL(tuner, seekCH())
        .WillRepeatedly(::testing::Return("5"));

    std::vector<std::string> seekChannel;
    for (int i = 0; i < 10; i++) {
        std::string seekCh = tuner.seekCH();
        if (seekCh.empty()) break; // assumeTrue equivalent
        int ch = std::stoi(seekCh);
        EXPECT_TRUE(0 <= ch && ch <= 99);
        seekChannel.push_back(seekCh);
    }
    EXPECT_EQ(10u, seekChannel.size());
}

// 채널검색테스트 : 시작 채널값을 99로 지정하고 10개 채널 검색 테스트
TEST_F(TunerTest, testSeekCh10timesAfterSetCH) {
    EXPECT_CALL(tuner, setCH("99"));
    EXPECT_CALL(tuner, seekCH())
        .WillRepeatedly(::testing::Return("50"));

    std::vector<std::string> seekChannel;
    tuner.setCH("99");
    for (int i = 0; i < 10; i++) {
        std::string seekCh = tuner.seekCH();
        if (seekCh.empty()) break; // assumeTrue equivalent
        int ch = std::stoi(seekCh);
        EXPECT_TRUE(0 <= ch && ch <= 99);
        seekChannel.push_back(seekCh);
    }
    EXPECT_EQ(10u, seekChannel.size());
}
