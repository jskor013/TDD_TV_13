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

#include "Tuner.h"
#include "remoteKey.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class TVController {
private:
    Tuner* tuner;
    std::string processingCH;
    std::vector<int> favoriteChannels;
    std::vector<int> searchedChannels;

    int getCurrentChannelInt() const {
        return std::stoi(tuner->getCurrentCH());
    }

    void clearBuffer() {
        processingCH.clear();
    }

    void commitChannel(const std::string& ch) {
        int val = std::stoi(ch);
        if (val < 0 || val > 99) {
            throw std::invalid_argument("Invalid channel");
        }
        std::string normalized = std::to_string(val);
        std::cout << "현재 설정하는 채널 : " << normalized << std::endl;
        tuner->setCH(normalized);
    }

    void commitBufferedDigits() {
        if (processingCH.empty()) {
            return;
        }
        commitChannel(processingCH);
        clearBuffer();
    }

    void handleDigit(remoteKey key) {
        processingCH += digitFromKey(key);
        if (processingCH.size() >= 2) {
            commitBufferedDigits();
        }
    }

    void handleOk() {
        if (processingCH.empty()) {
            return;
        }
        commitBufferedDigits();
    }

    void clearBufferBeforeNonDigitAction() {
        clearBuffer();
    }

    void channelUpWithoutSearch() {
        int next = (getCurrentChannelInt() + 1) % 100;
        commitChannel(std::to_string(next));
    }

    void channelDownWithoutSearch() {
        int next = (getCurrentChannelInt() + 99) % 100;
        commitChannel(std::to_string(next));
    }

    void channelUpWithSearch() {
        int current = getCurrentChannelInt();
        int next = -1;
        for (int ch : searchedChannels) {
            if (ch > current) {
                next = ch;
                break;
            }
        }
        if (next < 0) {
            next = searchedChannels.front();
        }
        commitChannel(std::to_string(next));
    }

    void channelDownWithSearch() {
        int current = getCurrentChannelInt();
        int prev = -1;
        for (auto it = searchedChannels.rbegin(); it != searchedChannels.rend(); ++it) {
            if (*it < current) {
                prev = *it;
                break;
            }
        }
        if (prev < 0) {
            prev = searchedChannels.back();
        }
        commitChannel(std::to_string(prev));
    }

    void handleChannelUp() {
        clearBufferBeforeNonDigitAction();
        if (searchedChannels.empty()) {
            channelUpWithoutSearch();
        } else {
            channelUpWithSearch();
        }
    }

    void handleChannelDown() {
        clearBufferBeforeNonDigitAction();
        if (searchedChannels.empty()) {
            channelDownWithoutSearch();
        } else {
            channelDownWithSearch();
        }
    }

    void handleSearch() {
        clearBufferBeforeNonDigitAction();
        searchedChannels.clear();
        while (true) {
            std::string ch = tuner->seekCH();
            if (ch.empty()) {
                break;
            }
            searchedChannels.push_back(std::stoi(ch));
        }
    }

    void toggleFavorite() {
        clearBufferBeforeNonDigitAction();
        int current = getCurrentChannelInt();
        auto it = std::find(favoriteChannels.begin(), favoriteChannels.end(), current);
        if (it != favoriteChannels.end()) {
            favoriteChannels.erase(it);
        } else {
            favoriteChannels.push_back(current);
            std::sort(favoriteChannels.begin(), favoriteChannels.end());
        }
    }

    void handleFavNext() {
        clearBufferBeforeNonDigitAction();
        if (favoriteChannels.empty()) {
            return;
        }
        int current = getCurrentChannelInt();
        int next = -1;
        for (int fav : favoriteChannels) {
            if (fav > current) {
                next = fav;
                break;
            }
        }
        if (next < 0) {
            next = favoriteChannels.front();
        }
        commitChannel(std::to_string(next));
    }

public:
    explicit TVController(Tuner* tuner) : tuner(tuner), processingCH("") {}

    void pushButton(remoteKey key) {
        if (isDigitKey(key)) {
            handleDigit(key);
            return;
        }

        switch (key) {
            case remoteKey::KEY_OK:
                handleOk();
                break;
            case remoteKey::KEY_CH_UP:
                handleChannelUp();
                break;
            case remoteKey::KEY_CH_DOWN:
                handleChannelDown();
                break;
            case remoteKey::KEY_SEARCH:
                handleSearch();
                break;
            case remoteKey::KEY_FAV_ADD:
                toggleFavorite();
                break;
            case remoteKey::KEY_FAV_NEXT:
                handleFavNext();
                break;
            default:
                break;
        }
    }
};

#endif // TV_CONTROLLER_H
