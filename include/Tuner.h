/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef TUNER_H
#define TUNER_H

#include <string>
#include <stdexcept>

class Tuner {
public:
    virtual ~Tuner() = default;
    virtual std::string seekCH() = 0;
    virtual void setCH(const std::string& ch) = 0;
    virtual std::string getCurrentCH() = 0;
};

#endif // TUNER_H
