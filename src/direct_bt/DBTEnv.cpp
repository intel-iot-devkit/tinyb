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

std::string DBTEnv::getProperty(const std::string & name) {
    const char * value = getenv(name.c_str());
    if( nullptr != value ) {
        return std::string( value );
    } else {
        const std::string name2 = "jvm." + name;
        const char * value2 = getenv(name2.c_str());
        if( nullptr != value2 ) {
            return std::string( value2 );
        } else {
            return std::string();
        }
    }
}

std::string DBTEnv::getProperty(const std::string & name, const std::string & default_value) {
    const std::string value = getProperty(name);
    if( 0 == value.length() ) {
        PLAIN_PRINT("DBTEnv::getProperty %s -> null -> %s (default)", name.c_str(), default_value.c_str());
        return default_value;
    } else {
        PLAIN_PRINT("DBTEnv::getProperty %s (default %s) -> %s", name.c_str(), default_value.c_str(), value.c_str());
        return value;
    }
}

bool DBTEnv::getBooleanProperty(const std::string & name, const bool default_value) {
    const std::string value = getProperty(name);
    if( 0 == value.length() ) {
        PLAIN_PRINT("DBTEnv::getBooleanProperty %s -> null -> %d (default)", name.c_str(), default_value);
        return default_value;
    } else {
        const bool res = "true" == value;
        PLAIN_PRINT("DBTEnv::getBooleanProperty %s (default %d) -> %d/%s", name.c_str(), default_value, res, value.c_str());
        return res;
    }
}

static void _envset(std::string prefix, std::string basename) {
    trimInPlace(basename);
    if( basename.length() > 0 ) {
        std::string name = prefix+"."+basename;
        PLAIN_PRINT("DBTEnv::setProperty %s -> true", name.c_str());
        setenv(name.c_str(), "true", 1 /* overwrite */);
    }
}

static void _env_explode_set(std::string prefix, std::string list) {
    size_t pos = 0, start = 0;
    while( (pos = list.find(',', start)) != std::string::npos ) {
        const size_t elem_len = pos-start; // excluding ','
        _envset(prefix, list.substr(start, elem_len));
        start = pos+1; // skip ','
    }
    const size_t elem_len = list.length()-start; // last one
    if( elem_len > 0 ) {
        _envset(prefix, list.substr(start, elem_len));
    }
}

static bool _env_explode_set(const std::string & name) {
    std::string value = DBTEnv::getProperty(name, "false");
    if( "false" == value ) {
        return false;
    }
    if( "true" == value ) {
        return true;
    }
    _env_explode_set("direct_bt.debug", value);
    return true;
}

DBTEnv::DBTEnv()
: DEBUG( _env_explode_set("direct_bt.debug") ),
  VERBOSE( _env_explode_set("direct_bt.verbose") || DBTEnv::DEBUG )
{
}
