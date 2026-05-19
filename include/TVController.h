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
#include "ChannelNumber.h"
#include "ChannelObserver.h"
#include "DigitInputBuffer.h"
#include "FavoriteStore.h"
#include "SearchSession.h"
#include "Tuner.h"
#include "remoteKey.h"
#include <unordered_map>
#include <variant>

// Facade: orchestrates digit input, favorites, search session, navigation, and channel commit.
class TVController {
public:
    explicit TVController(Tuner& tuner);

    void pushButton(remoteKey key);

private:
    using KeyHandler = void (TVController::*)();
    using ChannelNavigatorVariant =
        std::variant<tv::navigation::LinearChannelNavigator, tv::navigation::ListChannelNavigator>;

    Tuner& tuner_;
    tv::observer::CoutChannelObserver defaultObserver_;
    tv::observer::IChannelObserver* channelObserver_;
    tv::input::DigitInputBuffer digitBuffer_;
    tv::favorite::FavoriteStore favorites_;
    tv::search::SearchSession search_;

    tv::channel::ChannelNumber currentChannel() const;
    void commitChannel(tv::channel::ChannelNumber ch);
    void commitBufferedDigits();

    void handleDigit(remoteKey key);
    void handleOk();
    ChannelNavigatorVariant channelNavigatorForUpDown() const;
    void handleChannelUp();
    void handleChannelDown();
    void handleSearch();
    void toggleFavorite();
    void handleFavNext();

    static const std::unordered_map<remoteKey, KeyHandler>& keyHandlers();
    void handleUnsupportedKey(remoteKey key) const;
};

#endif // TV_CONTROLLER_H
