/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#include "TVController.h"
#include "ChannelNumber.h"
#include <cassert>

tv::channel::ChannelNumber TVController::currentChannel() const {
    return tv::channel::ChannelNumber::fromTunerString(tuner_.getCurrentCH());
}

void TVController::commitChannel(tv::channel::ChannelNumber ch) {
    const std::string normalized = ch.toTunerString();
    channelObserver_->onChannelCommitted(normalized);
    tuner_.setCH(normalized);
}

void TVController::commitBufferedDigits() {
    if (digitBuffer_.empty()) {
        return;
    }
    commitChannel(tv::channel::ChannelNumber::fromDigits(digitBuffer_.digits()));
    digitBuffer_.clear();
}

void TVController::handleDigit(remoteKey key) {
    digitBuffer_.appendDigit(digitFromKey(key));
    if (digitBuffer_.readyToAutoCommit()) {
        commitBufferedDigits();
    }
}

void TVController::handleOk() {
    if (digitBuffer_.empty()) {
        return;
    }
    commitBufferedDigits();
}

TVController::ChannelNavigatorVariant TVController::channelNavigatorForUpDown() const {
    if (search_.empty()) {
        return tv::navigation::LinearChannelNavigator{};
    }
    return tv::navigation::ListChannelNavigator{search_.channels()};
}

void TVController::handleChannelUp() {
    digitBuffer_.clear();
    const int current = currentChannel().value();
    const int next = std::visit(
        [&](const tv::navigation::IChannelNavigator& nav) { return nav.channelUp(current); },
        channelNavigatorForUpDown());
    commitChannel(tv::channel::ChannelNumber(next));
}

void TVController::handleChannelDown() {
    digitBuffer_.clear();
    const int current = currentChannel().value();
    const int prev = std::visit(
        [&](const tv::navigation::IChannelNavigator& nav) { return nav.channelDown(current); },
        channelNavigatorForUpDown());
    commitChannel(tv::channel::ChannelNumber(prev));
}

void TVController::handleSearch() {
    digitBuffer_.clear();
    search_.collect(tuner_);
}

void TVController::toggleFavorite() {
    digitBuffer_.clear();
    favorites_.toggle(currentChannel().value());
}

void TVController::handleFavNext() {
    digitBuffer_.clear();
    const auto next = favorites_.nextAfter(currentChannel().value());
    if (!next) {
        return;
    }
    commitChannel(tv::channel::ChannelNumber(*next));
}

const std::unordered_map<remoteKey, TVController::KeyHandler>& TVController::keyHandlers() {
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

void TVController::handleUnsupportedKey(remoteKey key) const {
    (void)key;
    assert(remote_key_detail::isValidKey(key) && "unsupported remoteKey: no handler registered");
}

TVController::TVController(Tuner& tuner)
    : tuner_(tuner)
    , channelObserver_(&defaultObserver_)
    , digitBuffer_()
    , favorites_()
    , search_() {}

void TVController::pushButton(remoteKey key) {
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
