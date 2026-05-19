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

#include "ChannelNavigator.h"
#include "Tuner.h"
#include "remoteKey.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class TVController {
private:
    using KeyHandler = void (TVController::*)();
    using ChannelNavigatorVariant =
        std::variant<tv::navigation::LinearChannelNavigator, tv::navigation::ListChannelNavigator>;

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

    ChannelNavigatorVariant channelNavigatorForUpDown() const {
        if (searchedChannels.empty()) {
            return tv::navigation::LinearChannelNavigator{};
        }
        return tv::navigation::ListChannelNavigator{searchedChannels};
    }

    void handleChannelUp() {
        clearBuffer();
        const int current = getCurrentChannelInt();
        const int next = std::visit(
            [&](const tv::navigation::IChannelNavigator& nav) { return nav.channelUp(current); },
            channelNavigatorForUpDown());
        commitChannel(std::to_string(next));
    }

    void handleChannelDown() {
        clearBuffer();
        const int current = getCurrentChannelInt();
        const int prev = std::visit(
            [&](const tv::navigation::IChannelNavigator& nav) { return nav.channelDown(current); },
            channelNavigatorForUpDown());
        commitChannel(std::to_string(prev));
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
        const tv::navigation::ListChannelNavigator nav(favoriteChannels);
        const int current = getCurrentChannelInt();
        commitChannel(std::to_string(nav.channelUp(current)));
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
