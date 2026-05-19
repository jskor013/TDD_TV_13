/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef FAVORITE_STORE_H
#define FAVORITE_STORE_H

#include "ChannelNavigator.h"
#include <algorithm>
#include <optional>
#include <vector>

namespace tv::favorite {

class FavoriteStore {
public:
    void toggle(int channel) {
        auto it = std::find(channels_.begin(), channels_.end(), channel);
        if (it != channels_.end()) {
            channels_.erase(it);
        } else {
            channels_.push_back(channel);
            std::sort(channels_.begin(), channels_.end());
        }
    }

    std::optional<int> nextAfter(int current) const {
        if (channels_.empty()) {
            return std::nullopt;
        }
        const tv::navigation::ListChannelNavigator nav(channels_);
        return nav.channelUp(current);
    }

    bool empty() const { return channels_.empty(); }

    const std::vector<int>& channels() const { return channels_; }

private:
    std::vector<int> channels_;
};

} // namespace tv::favorite

#endif // FAVORITE_STORE_H
