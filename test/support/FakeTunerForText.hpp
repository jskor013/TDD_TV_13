#ifndef FAKE_TUNER_FOR_TEXT_HPP
#define FAKE_TUNER_FOR_TEXT_HPP

#include "Tuner.h"
#include <deque>
#include <string>

class FakeTunerForText : public Tuner {
public:
    explicit FakeTunerForText(std::string current = "0") : current_(std::move(current)) {}

    void setSeekQueue(std::initializer_list<std::string> channels) {
        seekQueue_.assign(channels.begin(), channels.end());
    }

    std::string seekCH() override {
        if (seekQueue_.empty()) {
            return {};
        }
        std::string ch = seekQueue_.front();
        seekQueue_.pop_front();
        if (!ch.empty()) {
            current_ = ch;
        }
        return ch;
    }

    void setCH(const std::string& ch) override { current_ = ch; }

    std::string getCurrentCH() override { return current_; }

private:
    std::string current_;
    std::deque<std::string> seekQueue_;
};

#endif
