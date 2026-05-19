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
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
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

    void clearBufferBeforeNonDigitAction() {
        clearBuffer();
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
        clearBufferBeforeNonDigitAction();
        if (searchedChannels.empty()) {
            channelUpWithoutSearch();
        } else {
            channelUpWithSearch();
        }
    }

    void handleChannelDown() {
        clearBufferBeforeNonDigitAction();
        if (searchedChannels.empty()) {
            channelDownWithoutSearch();
        } else {
            channelDownWithSearch();
        }
    }

    void handleSearch() {
        clearBufferBeforeNonDigitAction();
        searchedChannels.clear();
        for (std::string ch = tuner->seekCH(); !ch.empty(); ch = tuner->seekCH()) {
            searchedChannels.push_back(std::stoi(ch));
        }
    }

    void toggleFavorite() {
        clearBufferBeforeNonDigitAction();
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
        clearBufferBeforeNonDigitAction();
        if (favoriteChannels.empty()) {
            return;
        }
        const int current = getCurrentChannelInt();
        const auto next = cyclicNeighbor(current, favoriteChannels, CyclicDirection::Forward);
        commitChannel(std::to_string(*next));
    }

public:
    explicit TVController(Tuner* tuner) : tuner(tuner), processingCH("") {}

    void pushButton(remoteKey key) {
        if (isDigitKey(key)) {
            handleDigit(key);
            return;
        }

        switch (key) {
            case remoteKey::KEY_OK:
                handleOk();
                break;
            case remoteKey::KEY_CH_UP:
                handleChannelUp();
                break;
            case remoteKey::KEY_CH_DOWN:
                handleChannelDown();
                break;
            case remoteKey::KEY_SEARCH:
                handleSearch();
                break;
            case remoteKey::KEY_FAV_ADD:
                toggleFavorite();
                break;
            case remoteKey::KEY_FAV_NEXT:
                handleFavNext();
                break;
            default:
                break;
        }
    }
};

#endif // TV_CONTROLLER_H
