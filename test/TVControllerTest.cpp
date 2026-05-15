#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Tuner.h"
#include "TVController.h"

class MockTunerForController : public Tuner {
public:
    MOCK_METHOD(std::string, seekCH, (), (override));
    MOCK_METHOD(void, setCH, (const std::string& ch), (override));
    MOCK_METHOD(std::string, getCurrentCH, (), (override));
};

TEST(TVControllerTest, testFramework) {
    // fail();
}
