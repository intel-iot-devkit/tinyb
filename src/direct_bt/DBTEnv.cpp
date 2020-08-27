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
        return std::string();
    }
}

std::string DBTEnv::getProperty(const std::string & name, const std::string & default_value) {
    const std::string value = getProperty(name);
    if( 0 == value.length() ) {
        PLAIN_PRINT("DBTEnv::getProperty %s: null -> %s (default)", name.c_str(), default_value.c_str());
        return default_value;
    } else {
        PLAIN_PRINT("DBTEnv::getProperty %s (default %s): %s", name.c_str(), default_value.c_str(), value.c_str());
        return value;
    }
}

bool DBTEnv::getBooleanProperty(const std::string & name, const bool default_value) {
    const std::string value = getProperty(name);
    if( 0 == value.length() ) {
        PLAIN_PRINT("DBTEnv::getBooleanProperty %s: null -> %d (default)", name.c_str(), default_value);
        return default_value;
    } else {
        const bool res = "true" == value;
        PLAIN_PRINT("DBTEnv::getBooleanProperty %s (default %d): %d/%s", name.c_str(), default_value, res, value.c_str());
        return res;
    }
}

#include <limits.h>

int32_t DBTEnv::getInt32Property(const std::string & name, const int32_t default_value,
                                 const int32_t min_allowed, const int32_t max_allowed)
{
    const std::string value = getProperty(name);
    if( 0 == value.length() ) {
        PLAIN_PRINT("DBTEnv::getInt32Property %s: null -> %" PRId32 " (default)", name.c_str(), default_value);
        return default_value;
    } else {
        int32_t res = default_value;
        char *endptr = NULL;
        const long int res0 = strtol(value.c_str(), &endptr, 10);
        if( *endptr == '\0' ) {
            // string value completely valid
            if( INT32_MIN <= res0 && res0 <= INT32_MAX ) {
                // matching int32_t value range
                const int32_t res1 = (int32_t)res0;
                if( min_allowed <= res1 && res1 <= max_allowed ) {
                    // matching user value range
                    res = res1;
                    PLAIN_PRINT("DBTEnv::getInt32Property %s (default %" PRId32 "): %" PRId32 "/%s",
                            name.c_str(), default_value, res, value.c_str());
                } else {
                    // invalid user value range
                    ERR_PRINT("DBTEnv::getInt32Property %s: %" PRId32 "/%s (invalid user range [% " PRId32 "..%" PRId32 "]) -> %" PRId32 " (default)",
                            name.c_str(), res1, value.c_str(), min_allowed, max_allowed, res);
                }
            } else {
                // invalid int32_t range
                ERR_PRINT("DBTEnv::getInt32Property %s: %" PRIu64 "/%s (invalid int32_t range) -> %" PRId32 " (default)",
                        name.c_str(), (uint64_t)res0, value.c_str(), res);
            }
        } else {
            // string value not fully valid
            ERR_PRINT("DBTEnv::getInt32Property %s: %s (invalid string) -> %" PRId32 " (default)",
                    name.c_str(), value.c_str(), res);
        }
        return res;
    }
}

uint32_t DBTEnv::getUint32Property(const std::string & name, const uint32_t default_value,
                                   const uint32_t min_allowed, const uint32_t max_allowed)
{
    const std::string value = getProperty(name);
    if( 0 == value.length() ) {
        PLAIN_PRINT("DBTEnv::getUint32Property %s: null -> %" PRIu32 " (default)", name.c_str(), default_value);
        return default_value;
    } else {
        uint32_t res = default_value;
        char *endptr = NULL;
        unsigned long int res0 = strtoul(value.c_str(), &endptr, 10);
        if( *endptr == '\0' ) {
            // string value completely valid
            if( res0 <= UINT32_MAX ) {
                // matching uint32_t value range
                const uint32_t res1 = (uint32_t)res0;
                if( min_allowed <= res1 && res1 <= max_allowed ) {
                    // matching user value range
                    res = res1;
                    PLAIN_PRINT("DBTEnv::getUint32Property %s (default %" PRIu32 "): %" PRIu32 "/%s",
                            name.c_str(), default_value, res, value.c_str());
                } else {
                    // invalid user value range
                    ERR_PRINT("DBTEnv::getUint32Property %s: %" PRIu32 "/%s (invalid user range [% " PRIu32 "..%" PRIu32 "]) -> %" PRIu32 " (default)",
                            name.c_str(), res1, value.c_str(), min_allowed, max_allowed, res);
                }
            } else {
                // invalid uint32_t range
                ERR_PRINT("DBTEnv::getUint32Property %s: %" PRIu64 "/%s (invalid uint32_t range) -> %" PRIu32 " (default)",
                        name.c_str(), (uint64_t)res0, value.c_str(), res);
            }
        } else {
            // string value not fully valid
            ERR_PRINT("DBTEnv::getUint32Property %s: %s (invalid string) -> %" PRIu32 " (default)",
                    name.c_str(), value.c_str(), res);
        }
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
