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
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_OK,
    KEY_CH_UP,
    KEY_CH_DOWN,
    KEY_SEARCH,
    KEY_FAV_ADD,
    KEY_FAV_NEXT
};

inline std::string to_string(remoteKey key) {
    switch (key) {
        case remoteKey::KEY_0: return "0";
        case remoteKey::KEY_1: return "1";
        case remoteKey::KEY_2: return "2";
        case remoteKey::KEY_3: return "3";
        case remoteKey::KEY_4: return "4";
        case remoteKey::KEY_5: return "5";
        case remoteKey::KEY_6: return "6";
        case remoteKey::KEY_7: return "7";
        case remoteKey::KEY_8: return "8";
        case remoteKey::KEY_9: return "9";
        case remoteKey::KEY_OK: return "OK";
        case remoteKey::KEY_CH_UP: return "CH_UP";
        case remoteKey::KEY_CH_DOWN: return "CH_DOWN";
        case remoteKey::KEY_SEARCH: return "SEARCH";
        case remoteKey::KEY_FAV_ADD: return "FAV_ADD";
        case remoteKey::KEY_FAV_NEXT: return "FAV_NEXT";
    }
    return "";
}

inline bool isDigitKey(remoteKey key) {
    return key >= remoteKey::KEY_0 && key <= remoteKey::KEY_9;
}

inline char digitFromKey(remoteKey key) {
    return static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(remoteKey::KEY_0)));
}

#endif // REMOTE_KEY_H
