/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef DIGIT_INPUT_BUFFER_H
#define DIGIT_INPUT_BUFFER_H

#include "ChannelNavigator.h"
#include <string>

namespace tv::input {

class DigitInputBuffer {
public:
    void clear() { digits_.clear(); }

    bool empty() const { return digits_.empty(); }

    const std::string& digits() const { return digits_; }

    void appendDigit(char digit) { digits_ += digit; }

    bool readyToAutoCommit() const { return digits_.size() >= tv::channel::kMaxDigitLen; }

private:
    std::string digits_;
};

} // namespace tv::input

#endif // DIGIT_INPUT_BUFFER_H
