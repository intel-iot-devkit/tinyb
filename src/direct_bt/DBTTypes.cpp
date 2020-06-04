/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cstdio>

#include <algorithm>

// #define PERF_PRINT_ON 1
// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "DBTTypes.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
}

using namespace direct_bt;

// *************************************************
// *************************************************
// *************************************************

#define CASE_TO_STRING(V) case V: return #V;
#define CASE2_TO_STRING(U,V) case U::V: return #V;

#define SETTING_ENUM(X) \
    X(AdapterSetting,NONE) \
    X(AdapterSetting,POWERED) \
    X(AdapterSetting,CONNECTABLE) \
    X(AdapterSetting,FAST_CONNECTABLE) \
    X(AdapterSetting,DISCOVERABLE) \
    X(AdapterSetting,BONDABLE) \
    X(AdapterSetting,LINK_SECURITY) \
    X(AdapterSetting,SSP) \
    X(AdapterSetting,BREDR) \
    X(AdapterSetting,HS) \
    X(AdapterSetting,LE) \
    X(AdapterSetting,ADVERTISING) \
    X(AdapterSetting,SECURE_CONN) \
    X(AdapterSetting,DEBUG_KEYS) \
    X(AdapterSetting,PRIVACY) \
    X(AdapterSetting,CONFIGURATION) \
    X(AdapterSetting,STATIC_ADDRESS) \
    X(AdapterSetting,PHY_CONFIGURATION)

std::string direct_bt::getAdapterSettingBitString(const AdapterSetting settingBit) {
    switch(settingBit) {
        SETTING_ENUM(CASE2_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown Setting Bit";
}

std::string direct_bt::getAdapterSettingsString(const AdapterSetting settingMask) {
    const uint32_t one = 1;
    bool has_pre = false;
    std::string out("[");
    for(int i=0; i<32; i++) {
        const AdapterSetting settingBit = static_cast<AdapterSetting>( one << i );
        if( AdapterSetting::NONE != ( settingMask & settingBit ) ) {
            if( has_pre ) { out.append(", "); }
            out.append(getAdapterSettingBitString(settingBit));
            has_pre = true;
        }
    }
    out.append("]");
    return out;
}
