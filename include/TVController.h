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
#include <string>
#include <iostream>

class TVController {
private:
    Tuner* tuner;
    std::string processingCH;

    void setTunerCh() {
        // 로그는 테스트의 결과가 절대 아닙니다. 로그가 있는 것을 테스트로 간주하지 마시기 바랍니다.
        std::cout << "현재 설정하는 채널 : " << processingCH << std::endl;
        // tuner->setCH(processingCH);
    }

public:
    explicit TVController(Tuner* tuner) : tuner(tuner), processingCH("") {}

    void pushButton(remoteKey key) {
        std::string tmp;
        switch (key) {
            case remoteKey::KEY_1:
            case remoteKey::KEY_2:
            case remoteKey::KEY_3:
            case remoteKey::KEY_4:
            case remoteKey::KEY_5:
            case remoteKey::KEY_6:
            case remoteKey::KEY_7:
            case remoteKey::KEY_8:
            case remoteKey::KEY_9:
            case remoteKey::KEY_0:                  
                processingCH += to_string(key);
                if(processingCH.size()==2) 
                {
                    tuner->setCH(processingCH);
                    processingCH = "";
                }
                break;            
            case remoteKey::KEY_OK:
                tuner->setCH(processingCH);
                break;
            case remoteKey::KEY_OTHER:
                processingCH = "";
                break;
        }
    }
};

#endif // TV_CONTROLLER_H
