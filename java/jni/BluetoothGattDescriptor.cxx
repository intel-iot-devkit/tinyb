/*
 * Author: Andrei Vasiliu <andrei.vasiliu@intel.com>
 * Copyright (c) 2016 Intel Corporation.
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

#include "tinyb/BluetoothGattCharacteristic.hpp"
#include "tinyb/BluetoothGattDescriptor.hpp"
#include "tinyb/BluetoothObject.hpp"

#include "tinyb_BluetoothGattDescriptor.h"

#include "helper.hpp"

using namespace tinyb;

jobject Java_tinyb_BluetoothGattDescriptor_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "GATT_DESCRIPTOR");
}

jobject Java_tinyb_BluetoothGattDescriptor_clone(JNIEnv *env, jobject obj)
{
    return generic_clone<BluetoothGattDescriptor>(env, obj, "BluetoothGattDescriptor");
}

jbyteArray Java_tinyb_BluetoothGattDescriptor_readValue(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    std::vector<unsigned char> array = obj_gatt_desc->read_value();
    unsigned int array_size = array.size();

    jbyteArray result = env->NewByteArray((jsize)array_size);
    env->SetByteArrayRegion(result, 0, (jsize)array_size, (const jbyte *)&array[0]);

    return result;
}

jboolean Java_tinyb_BluetoothGattDescriptor_writeValue(JNIEnv *env, jobject obj, jbyteArray argValue)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);

    jboolean is_copy = false;
    jbyte *native_array = env->GetByteArrayElements(argValue, &is_copy);
    jsize native_array_length = env->GetArrayLength(argValue);

    std::vector<unsigned char> array(native_array_length);
    for (int i = 0; i < native_array_length; ++i)
    {
        array.push_back(native_array[i]);
    }

    return obj_gatt_desc->write_value(array);
}

jstring Java_tinyb_BluetoothGattDescriptor_getUuid(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    std::string uuid = obj_gatt_desc->get_uuid();

    return env->NewStringUTF((const char *)uuid.c_str());
}

jobject Java_tinyb_BluetoothGattDescriptor_getCharacteristic(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    BluetoothGattCharacteristic *obj_gatt_char = obj_gatt_desc->get_characteristic().clone();

    jclass b_gatt_char_class = search_class(env, *obj_gatt_char);
    jmethodID b_gatt_char_ctor = search_method(env, b_gatt_char_class, "<init>",
                                                "(J)V", false);
    jobject result = env->NewObject(b_gatt_char_class, b_gatt_char_ctor, (jlong)obj_gatt_char);
    if (result == NULL)
    {
        throw std::runtime_error("cannot create instance of class\n");
    }

    return result;
}

jbyteArray Java_tinyb_BluetoothGattDescriptor_getValue(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    std::vector<unsigned char> array = obj_gatt_desc->get_value();
    unsigned int array_size = array.size();

    jbyteArray result = env->NewByteArray((jsize)array_size);
    env->SetByteArrayRegion(result, 0, (jsize)array_size, (const jbyte *)&array[0]);

    return result;

}

void Java_tinyb_BluetoothGattDescriptor_delete(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    delete obj_gatt_desc;
}

