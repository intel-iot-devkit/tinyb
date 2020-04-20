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

#include <dbt_debug.hpp>
#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cstdio>

#include  <algorithm>

#include "GATTTypes.hpp"


using namespace direct_bt;

#define CHAR_DECL_PROPS_ENUM(X) \
        X(Broadcast) \
        X(Read) \
        X(WriteNoAck) \
        X(WriteWithAck) \
        X(Notify) \
        X(Indicate) \
        X(AuthSignedWrite) \
        X(ExtProps)

#define CASE_TO_STRING(V) case V: return #V;

std::string GATTCharacterisicsDecl::getPropertyString(const PropertyBitVal prop) {
    switch(prop) {
        CHAR_DECL_PROPS_ENUM(CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown property";
}

std::string GATTCharacterisicsDecl::getPropertiesString(const PropertyBitVal properties) {
    std::string res = "[";
    int i = 0;
    if( 0 != ( properties & Broadcast ) ) {
        i++;
        res += getPropertyString(Broadcast);
    }
    if( 0 != ( properties & Read ) ) {
        if( 0 < i++ ) {
            res += ", ";
        }
        res += getPropertyString(Read);
    }
    if( 0 != ( properties & WriteNoAck ) ) {
        if( 0 < i++ ) {
            res += ", ";
        }
        res += getPropertyString(WriteNoAck);
    }
    if( 0 != ( properties & WriteWithAck ) ) {
        if( 0 < i++ ) {
            res += ", ";
        }
        res += getPropertyString(WriteWithAck);
    }
    if( 0 != ( properties & Notify ) ) {
        if( 0 < i++ ) {
            res += ", ";
        }
        res += getPropertyString(Notify);
    }
    if( 0 != ( properties & Indicate ) ) {
        if( 0 < i++ ) {
            res += ", ";
        }
        res += getPropertyString(Indicate);
    }
    if( 0 != ( properties & AuthSignedWrite ) ) {
        if( 0 < i++ ) {
            res += ", ";
        }
        res += getPropertyString(AuthSignedWrite);
    }
    if( 0 != ( properties & ExtProps ) ) {
        if( 0 < i++ ) {
            res += ", ";
        }
        res += getPropertyString(ExtProps);
    }
    return res+"]";
}



