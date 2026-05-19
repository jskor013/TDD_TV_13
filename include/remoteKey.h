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

#include <array>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

#define REMOTE_KEY_LIST(X)                                                                       \
    X(KEY_0, "0", true, '0')                                                                     \
    X(KEY_1, "1", true, '1')                                                                     \
    X(KEY_2, "2", true, '2')                                                                     \
    X(KEY_3, "3", true, '3')                                                                     \
    X(KEY_4, "4", true, '4')                                                                     \
    X(KEY_5, "5", true, '5')                                                                     \
    X(KEY_6, "6", true, '6')                                                                     \
    X(KEY_7, "7", true, '7')                                                                     \
    X(KEY_8, "8", true, '8')                                                                     \
    X(KEY_9, "9", true, '9')                                                                     \
    X(KEY_OK, "OK", false, '\0')                                                                 \
    X(KEY_CH_UP, "CH_UP", false, '\0')                                                           \
    X(KEY_CH_DOWN, "CH_DOWN", false, '\0')                                                       \
    X(KEY_SEARCH, "SEARCH", false, '\0')                                                         \
    X(KEY_FAV_ADD, "FAV_ADD", false, '\0')                                                       \
    X(KEY_FAV_NEXT, "FAV_NEXT", false, '\0')

enum class remoteKey {
#define X(name, label, isDigit, digitChar) name,
    REMOTE_KEY_LIST(X)
#undef X
    KEY_COUNT
};

namespace remote_key_detail {

struct RemoteKeyMeta {
    std::string_view label;
    bool isDigit;
    char digitChar;
};

inline constexpr std::size_t kRemoteKeyCount = static_cast<std::size_t>(remoteKey::KEY_COUNT);

inline constexpr std::array<RemoteKeyMeta, kRemoteKeyCount> kRemoteKeyTable = {{
#define X(name, label, isDigit, digitChar) {label, isDigit, digitChar},
    REMOTE_KEY_LIST(X)
#undef X
}};

inline bool isValidKey(remoteKey key) {
    const auto idx = static_cast<std::size_t>(key);
    return idx < kRemoteKeyCount;
}

} // namespace remote_key_detail

[[nodiscard]] inline std::string to_string(remoteKey key) {
    if (!remote_key_detail::isValidKey(key)) {
        throw std::out_of_range("unknown remoteKey");
    }
    return std::string(remote_key_detail::kRemoteKeyTable[static_cast<std::size_t>(key)].label);
}

[[nodiscard]] inline bool isDigitKey(remoteKey key) {
    if (!remote_key_detail::isValidKey(key)) {
        return false;
    }
    return remote_key_detail::kRemoteKeyTable[static_cast<std::size_t>(key)].isDigit;
}

inline char digitFromKey(remoteKey key) {
    if (!remote_key_detail::isValidKey(key) ||
        !remote_key_detail::kRemoteKeyTable[static_cast<std::size_t>(key)].isDigit) {
        throw std::invalid_argument("not a digit key");
    }
    return remote_key_detail::kRemoteKeyTable[static_cast<std::size_t>(key)].digitChar;
}

#undef REMOTE_KEY_LIST

#endif // REMOTE_KEY_H
