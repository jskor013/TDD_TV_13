/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef CHANNEL_OBSERVER_H
#define CHANNEL_OBSERVER_H

#include <iostream>
#include <string>

namespace tv::observer {

class IChannelObserver {
public:
    virtual ~IChannelObserver() = default;
    virtual void onChannelCommitted(const std::string& normalizedChannel) = 0;
};

class CoutChannelObserver final : public IChannelObserver {
public:
    void onChannelCommitted(const std::string& normalizedChannel) override {
        std::cout << "현재 설정하는 채널 : " << normalizedChannel << std::endl;
    }
};

class NullChannelObserver final : public IChannelObserver {
public:
    void onChannelCommitted(const std::string&) override {}
};

} // namespace tv::observer

#endif // CHANNEL_OBSERVER_H
