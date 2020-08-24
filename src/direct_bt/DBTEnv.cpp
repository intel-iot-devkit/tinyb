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

#include "direct_bt/DBTEnv.hpp"
#include "direct_bt/dbt_debug.hpp"

using namespace direct_bt;

const uint64_t DBTEnv::startupTimeMilliseconds = direct_bt::getCurrentMilliseconds();

const char * DBTEnv::getProperty(const char *name) {
    const char * value = getenv(name);
    if( nullptr != value ) {
        return value;
    } else {
        char name2[strlen(name)+4+1];
        strcpy(name2, "jvm_");
        strcpy(name2+4, name);
        return getenv(name2);
    }
}

std::string DBTEnv::getProperty(const char *name, const std::string & default_value) {
    const char * value = getProperty(name);
    if( nullptr != value ) {
        PLAIN_PRINT("DBTEnv::getProperty %s (default %s) -> %s", name, default_value.c_str(), value);
        return value;
    } else {
        PLAIN_PRINT("DBTEnv::getProperty %s -> null -> %s (default)", name, default_value.c_str());
        return default_value;
    }
}

bool DBTEnv::getBooleanProperty(const char *name, const bool default_value) {
    const char * value = getProperty(name);
    if( nullptr != value ) {
        bool res = 0==strcmp("true", value);
        PLAIN_PRINT("DBTEnv::getBooleanProperty %s (default %d) -> %d/%s", name, default_value, res, value);
        return res;
    } else {
        PLAIN_PRINT("DBTEnv::getBooleanProperty %s -> null -> %d (default)", name, default_value);
        return default_value;
    }
}

DBTEnv::DBTEnv()
: DEBUG( DBTEnv::getBooleanProperty("direct_bt_debug", false) ),
  VERBOSE( DBTEnv::DEBUG || DBTEnv::getBooleanProperty("direct_bt_verbose", false) )
{
}
