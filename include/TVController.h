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
#include "ChannelObserver.h"
#include "DigitInputBuffer.h"
#include "FavoriteStore.h"
#include "SearchSession.h"
#include "Tuner.h"
#include "remoteKey.h"
#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

// Facade: orchestrates digit input, favorites, search session, navigation, and channel commit.
class TVController {
private:
    using KeyHandler = void (TVController::*)();
    using ChannelNavigatorVariant =
        std::variant<tv::navigation::LinearChannelNavigator, tv::navigation::ListChannelNavigator>;

    Tuner* tuner_;
    tv::observer::CoutChannelObserver defaultObserver_;
    tv::observer::IChannelObserver* channelObserver_;
    tv::input::DigitInputBuffer digitBuffer_;
    tv::favorite::FavoriteStore favorites_;
    tv::search::SearchSession search_;

    int getCurrentChannelInt() const {
        return std::stoi(tuner_->getCurrentCH());
    }

    void commitChannel(const std::string& ch) {
        int val = std::stoi(ch);
        if (val < tv::channel::kMin || val > tv::channel::kMax) {
            throw std::invalid_argument("Invalid channel");
        }
        const std::string normalized = std::to_string(val);
        channelObserver_->onChannelCommitted(normalized);
        tuner_->setCH(normalized);
    }

    void commitBufferedDigits() {
        if (digitBuffer_.empty()) {
            return;
        }
        commitChannel(digitBuffer_.digits());
        digitBuffer_.clear();
    }

    void handleDigit(remoteKey key) {
        digitBuffer_.appendDigit(digitFromKey(key));
        if (digitBuffer_.readyToAutoCommit()) {
            commitBufferedDigits();
        }
    }

    void handleOk() {
        if (digitBuffer_.empty()) {
            return;
        }
        commitBufferedDigits();
    }

    ChannelNavigatorVariant channelNavigatorForUpDown() const {
        if (search_.empty()) {
            return tv::navigation::LinearChannelNavigator{};
        }
        return tv::navigation::ListChannelNavigator{search_.channels()};
    }

    void handleChannelUp() {
        digitBuffer_.clear();
        const int current = getCurrentChannelInt();
        const int next = std::visit(
            [&](const tv::navigation::IChannelNavigator& nav) { return nav.channelUp(current); },
            channelNavigatorForUpDown());
        commitChannel(std::to_string(next));
    }

    void handleChannelDown() {
        digitBuffer_.clear();
        const int current = getCurrentChannelInt();
        const int prev = std::visit(
            [&](const tv::navigation::IChannelNavigator& nav) { return nav.channelDown(current); },
            channelNavigatorForUpDown());
        commitChannel(std::to_string(prev));
    }

    void handleSearch() {
        digitBuffer_.clear();
        search_.collect(*tuner_);
    }

    void toggleFavorite() {
        digitBuffer_.clear();
        favorites_.toggle(getCurrentChannelInt());
    }

    void handleFavNext() {
        digitBuffer_.clear();
        const auto next = favorites_.nextAfter(getCurrentChannelInt());
        if (!next) {
            return;
        }
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
    explicit TVController(Tuner* tuner)
        : tuner_(tuner)
        , channelObserver_(&defaultObserver_)
        , digitBuffer_()
        , favorites_()
        , search_() {}

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
