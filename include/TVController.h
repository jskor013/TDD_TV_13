/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef TV_CONTROLLER_H
#define TV_CONTROLLER_H

#include "Tuner.h"
#include "remoteKey.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
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

class TVController {
private:
    using KeyHandler = void (TVController::*)();

    enum class CyclicDirection { Forward, Backward };

    static std::optional<int> cyclicNeighbor(int current,
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

    Tuner* tuner;
    std::string processingCH;
    std::vector<int> favoriteChannels;
    std::vector<int> searchedChannels;

    int getCurrentChannelInt() const {
        return std::stoi(tuner->getCurrentCH());
    }

    void clearBuffer() {
        processingCH.clear();
    }

    void commitChannel(const std::string& ch) {
        int val = std::stoi(ch);
        if (val < tv::channel::kMin || val > tv::channel::kMax) {
            throw std::invalid_argument("Invalid channel");
        }
        std::string normalized = std::to_string(val);
        std::cout << "현재 설정하는 채널 : " << normalized << std::endl;
        tuner->setCH(normalized);
    }

    void commitBufferedDigits() {
        if (processingCH.empty()) {
            return;
        }
        commitChannel(processingCH);
        clearBuffer();
    }

    void handleDigit(remoteKey key) {
        processingCH += digitFromKey(key);
        if (processingCH.size() >= tv::channel::kMaxDigitLen) {
            commitBufferedDigits();
        }
    }

    void handleOk() {
        if (processingCH.empty()) {
            return;
        }
        commitBufferedDigits();
    }

    void channelUpWithoutSearch() {
        const int next = tv::channel::step(getCurrentChannelInt(), 1);
        commitChannel(std::to_string(next));
    }

    void channelDownWithoutSearch() {
        const int next = tv::channel::step(getCurrentChannelInt(), -1);
        commitChannel(std::to_string(next));
    }

    void channelUpWithSearch() {
        const int current = getCurrentChannelInt();
        const auto next = cyclicNeighbor(current, searchedChannels, CyclicDirection::Forward);
        commitChannel(std::to_string(*next));
    }

    void channelDownWithSearch() {
        const int current = getCurrentChannelInt();
        const auto prev = cyclicNeighbor(current, searchedChannels, CyclicDirection::Backward);
        commitChannel(std::to_string(*prev));
    }

    void handleChannelUp() {
        clearBuffer();
        if (searchedChannels.empty()) {
            channelUpWithoutSearch();
        } else {
            channelUpWithSearch();
        }
    }

    void handleChannelDown() {
        clearBuffer();
        if (searchedChannels.empty()) {
            channelDownWithoutSearch();
        } else {
            channelDownWithSearch();
        }
    }

    void handleSearch() {
        clearBuffer();
        searchedChannels.clear();
        for (std::string ch = tuner->seekCH(); !ch.empty(); ch = tuner->seekCH()) {
            searchedChannels.push_back(std::stoi(ch));
        }
    }

    void toggleFavorite() {
        clearBuffer();
        int current = getCurrentChannelInt();
        auto it = std::find(favoriteChannels.begin(), favoriteChannels.end(), current);
        if (it != favoriteChannels.end()) {
            favoriteChannels.erase(it);
        } else {
            favoriteChannels.push_back(current);
            std::sort(favoriteChannels.begin(), favoriteChannels.end());
        }
    }

    void handleFavNext() {
        clearBuffer();
        if (favoriteChannels.empty()) {
            return;
        }
        const int current = getCurrentChannelInt();
        const auto next = cyclicNeighbor(current, favoriteChannels, CyclicDirection::Forward);
        commitChannel(std::to_string(*next));
    }

    static const std::unordered_map<remoteKey, KeyHandler>& keyHandlers() {
        static const std::unordered_map<remoteKey, KeyHandler> handlers = {
            {remoteKey::KEY_OK, &TVController::handleOk},
            {remoteKey::KEY_CH_UP, &TVController::handleChannelUp},
            {remoteKey::KEY_CH_DOWN, &TVController::handleChannelDown},
            {remoteKey::KEY_SEARCH, &TVController::handleSearch},
            {remoteKey::KEY_FAV_ADD, &TVController::toggleFavorite},
            {remoteKey::KEY_FAV_NEXT, &TVController::handleFavNext},
        };
        return handlers;
    }

    // Policy (B3): unregistered keys are ignored (no-op). Debug builds trap invalid enumerators.
    void handleUnsupportedKey(remoteKey key) const {
        (void)key;
        assert(remote_key_detail::isValidKey(key) && "unsupported remoteKey: no handler registered");
    }

public:
    explicit TVController(Tuner* tuner) : tuner(tuner), processingCH("") {}

    void pushButton(remoteKey key) {
        if (isDigitKey(key)) {
            handleDigit(key);
            return;
        }

        const auto& handlers = keyHandlers();
        const auto it = handlers.find(key);
        if (it != handlers.end()) {
            (this->*(it->second))();
            return;
        }

        handleUnsupportedKey(key);
    }
};

#endif // TV_CONTROLLER_H
