/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef CHANNEL_NAVIGATOR_H
#define CHANNEL_NAVIGATOR_H

#include <optional>
#include <vector>

namespace tv::channel {
inline constexpr int kMin = 0;
inline constexpr int kMax = 99;
inline constexpr int kCount = 100;
inline constexpr int kMaxDigitLen = 2;

inline int step(int ch, int delta) {
    const int n = ch + delta;
    return ((n % kCount) + kCount) % kCount;
}
} // namespace tv::channel

namespace tv::navigation {

enum class CyclicDirection { Forward, Backward };

inline std::optional<int> cyclicNeighbor(int current,
                                       const std::vector<int>& channels,
                                       CyclicDirection dir) {
    if (channels.empty()) {
        return std::nullopt;
    }
    if (dir == CyclicDirection::Forward) {
        for (int ch : channels) {
            if (ch > current) {
                return ch;
            }
        }
        return channels.front();
    }
    for (auto it = channels.rbegin(); it != channels.rend(); ++it) {
        if (*it < current) {
            return *it;
        }
    }
    return channels.back();
}

class IChannelNavigator {
public:
    virtual ~IChannelNavigator() = default;
    virtual int channelUp(int current) const = 0;
    virtual int channelDown(int current) const = 0;
};

class LinearChannelNavigator final : public IChannelNavigator {
public:
    int channelUp(int current) const override { return tv::channel::step(current, 1); }
    int channelDown(int current) const override { return tv::channel::step(current, -1); }
};

class ListChannelNavigator final : public IChannelNavigator {
public:
    explicit ListChannelNavigator(const std::vector<int>& channels) : channels_(channels) {}

    int channelUp(int current) const override {
        return *cyclicNeighbor(current, channels_, CyclicDirection::Forward);
    }

    int channelDown(int current) const override {
        return *cyclicNeighbor(current, channels_, CyclicDirection::Backward);
    }

private:
    const std::vector<int>& channels_;
};

} // namespace tv::navigation

#endif // CHANNEL_NAVIGATOR_H
