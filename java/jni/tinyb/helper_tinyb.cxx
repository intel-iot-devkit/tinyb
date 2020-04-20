/*
 * Author: Andrei Vasiliu <andrei.vasiliu@intel.com>
 * Copyright (c) 2016 Intel Corporation.
 *
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

#include <jni.h>
#include <memory>
#include <stdexcept>
#include <vector>

#include "helper_tinyb.hpp"

using namespace tinyb;

jclass search_class(JNIEnv *env, BluetoothObject &object)
{
    return search_class(env, object.get_java_class().c_str());
}

BluetoothType from_int_to_btype(int type)
{
    BluetoothType result = BluetoothType::NONE;

    switch (type)
    {
        case 0:
            result = BluetoothType::NONE;
            break;

        case 1:
            result = BluetoothType::ADAPTER;
            break;

        case 2:
            result = BluetoothType::DEVICE;
            break;

        case 3:
            result = BluetoothType::GATT_SERVICE;
            break;

        case 4:
            result = BluetoothType::GATT_CHARACTERISTIC;
            break;

        case 5:
            result = BluetoothType::GATT_CHARACTERISTIC;
            break;

        default:
            result = BluetoothType::NONE;
            break;
    }

    return result;
}

TransportType from_int_to_transport_type(int type)
{
    TransportType result = TransportType::AUTO;

    switch (type)
    {
        case 0:
            result = TransportType::AUTO;
            break;

        case 1:
            result = TransportType::BREDR;
            break;

        case 2:
            result = TransportType::LE;
            break;

        default:
            result = TransportType::AUTO;
            break;
    }

    return result;
}

void raise_java_bluetooth_exception(JNIEnv *env, BluetoothException &e)
{
    env->ThrowNew(env->FindClass("org/tinyb/BluetoothException"), e.what());
}

