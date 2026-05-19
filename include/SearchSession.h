/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef SEARCH_SESSION_H
#define SEARCH_SESSION_H

#include "Tuner.h"
#include <string>
#include <vector>

namespace tv::search {

class SearchSession {
public:
    void clear() { channels_.clear(); }

    bool empty() const { return channels_.empty(); }

    const std::vector<int>& channels() const { return channels_; }

    void collect(Tuner& tuner) {
        channels_.clear();
        for (std::string ch = tuner.seekCH(); !ch.empty(); ch = tuner.seekCH()) {
            channels_.push_back(std::stoi(ch));
        }
    }

private:
    std::vector<int> channels_;
};

} // namespace tv::search

#endif // SEARCH_SESSION_H
