#pragma once
#include "Tuner.h"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <utility>

class FakeTuner : public Tuner {
    int current_ = 0;
    std::vector<int> available_;

public:
    explicit FakeTuner(std::vector<int> avail)
        : available_(std::move(avail)) {}

    // 현재보다 큰 채널 중 가장 작은 것 (없으면 wrap)
    std::string seekCH() override {
        auto it = std::find_if(
            available_.begin(), available_.end(),
            [&](int ch) { return ch > current_; });
        current_ = (it != available_.end())
            ? *it
            : available_.front();
        return std::to_string(current_);
    }
    
    void setCH(const std::string& ch) override {
        int v = std::stoi(ch);
        if (v < 0 || v > 99)
            throw std::invalid_argument(
                "채널 범위 초과: " + ch);
        current_ = v;
    }

    std::string getCurrentCH() override {
        return std::to_string(current_);
    }
};