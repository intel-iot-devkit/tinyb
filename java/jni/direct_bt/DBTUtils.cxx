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

#include "org_tinyb_BluetoothUtils.h"

#include <cstdint>
#include <cinttypes>

#include <time.h>

#include "helper_base.hpp"
#include "helper_dbt.hpp"

jstring Java_org_tinyb_BluetoothUtils_getUTF8String(JNIEnv *env, jclass clazz, jbyteArray jbuffer, jint offset, jint size) {
    (void)clazz;

    const int buffer_size = env->GetArrayLength(jbuffer);
    if( 0 == buffer_size ) {
        return env->NewStringUTF("");
    }
    if( buffer_size < offset+size ) {
        throw direct_bt::IllegalArgumentException("buffer.length "+std::to_string(buffer_size)+
                " < offset "+std::to_string(offset)+
                " + size "+std::to_string(size), E_FILE_LINE);
    }

    JNICriticalArray<uint8_t> criticalArray(env); // RAII - release
    uint8_t * buffer_ptr = criticalArray.get(jbuffer, criticalArray.Mode::NO_UPDATE_AND_RELEASE);
    if( NULL == buffer_ptr ) {
        throw direct_bt::InternalError("GetPrimitiveArrayCritical(byte array) is null", E_FILE_LINE);
    }
    std::string sres = direct_bt::getUTF8String(buffer_ptr+offset, size);

    return from_string_to_jstring(env, sres);
}
