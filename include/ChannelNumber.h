/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef CHANNEL_NUMBER_H
#define CHANNEL_NUMBER_H

#include "ChannelNavigator.h"
#include <stdexcept>
#include <string>

namespace tv::channel {

class ChannelNumber {
public:
    explicit ChannelNumber(int value);

    static ChannelNumber fromDigits(const std::string& digits);
    static ChannelNumber fromTunerString(const std::string& ch);

    int value() const { return value_; }
    std::string toTunerString() const;

private:
    static int parseAndValidate(const std::string& s);

    int value_;
};

inline ChannelNumber::ChannelNumber(int value) : value_(value) {
    if (value < kMin || value > kMax) {
        throw std::invalid_argument("Invalid channel");
    }
}

inline int ChannelNumber::parseAndValidate(const std::string& s) {
    const int val = std::stoi(s);
    if (val < kMin || val > kMax) {
        throw std::invalid_argument("Invalid channel");
    }
    return val;
}

inline ChannelNumber ChannelNumber::fromDigits(const std::string& digits) {
    return ChannelNumber(parseAndValidate(digits));
}

inline ChannelNumber ChannelNumber::fromTunerString(const std::string& ch) {
    return ChannelNumber(parseAndValidate(ch));
}

inline std::string ChannelNumber::toTunerString() const {
    return std::to_string(value_);
}

} // namespace tv::channel

#endif // CHANNEL_NUMBER_H
