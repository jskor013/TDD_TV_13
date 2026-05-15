/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef REMOTE_KEY_H
#define REMOTE_KEY_H

#include <string>

enum class remoteKey {
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_OK,
    KEY_OTHER
};

inline std::string to_string(remoteKey key) {
    switch (key) {
        case remoteKey::KEY_1: return "1";
        case remoteKey::KEY_OK: return "OK";
    }
    return "";
}

#endif // REMOTE_KEY_H
