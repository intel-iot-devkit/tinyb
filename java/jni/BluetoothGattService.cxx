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

#include "tinyb/BluetoothDevice.hpp"
#include "tinyb/BluetoothGattService.hpp"
#include "tinyb/BluetoothGattCharacteristic.hpp"
#include "tinyb/BluetoothObject.hpp"

#include "tinyb_BluetoothGattService.h"

#include "helper.hpp"

using namespace tinyb;

jobject Java_tinyb_BluetoothGattService_getBluetoothType(JNIEnv *env, jobject obj)
{
    try {
        (void)obj;

        return get_bluetooth_type(env, "GATT_SERVICE");
    } catch (std::bad_alloc &e) {
        raise_java_oom_exception(env, e);
    } catch (BluetoothException &e) {
        raise_java_bluetooth_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_runtime_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_invalid_arg_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
    return nullptr;
}

jobject Java_tinyb_BluetoothGattService_clone(JNIEnv *env, jobject obj)
{
    try {
        return generic_clone<BluetoothGattService>(env, obj);
    } catch (std::bad_alloc &e) {
        raise_java_oom_exception(env, e);
    } catch (BluetoothException &e) {
        raise_java_bluetooth_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_runtime_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_invalid_arg_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
    return nullptr;
}

jstring Java_tinyb_BluetoothGattService_getUUID(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
        std::string uuid = obj_gatt_serv->get_uuid();

        return env->NewStringUTF((const char *)uuid.c_str());
    } catch (std::bad_alloc &e) {
        raise_java_oom_exception(env, e);
    } catch (BluetoothException &e) {
        raise_java_bluetooth_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_runtime_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_invalid_arg_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
    return nullptr;
}

jobject Java_tinyb_BluetoothGattService_getDevice(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
        BluetoothDevice *obj_device = obj_gatt_serv->get_device().clone();

        jclass b_device_class = search_class(env, *obj_device);
        jmethodID b_device_ctor = search_method(env, b_device_class, "<init>",
                                                "(J)V", false);
        jobject result = env->NewObject(b_device_class, b_device_ctor, (jlong)obj_device);
        if (result == NULL)
        {
            throw std::runtime_error("cannot create instance of class\n");
        }

        return result;
    } catch (std::bad_alloc &e) {
        raise_java_oom_exception(env, e);
    } catch (BluetoothException &e) {
        raise_java_bluetooth_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_runtime_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_invalid_arg_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
    return nullptr;
}

jboolean Java_tinyb_BluetoothGattService_getPrimary(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);

        return obj_gatt_serv->get_primary() ? JNI_TRUE : JNI_FALSE;
    } catch (std::bad_alloc &e) {
        raise_java_oom_exception(env, e);
    } catch (BluetoothException &e) {
        raise_java_bluetooth_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_runtime_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_invalid_arg_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
    return JNI_FALSE;
}

jobject Java_tinyb_BluetoothGattService_getCharacteristics(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
        std::vector<std::unique_ptr<BluetoothGattCharacteristic>> array =
                                                    obj_gatt_serv->get_characteristics();
        jobject result = convert_vector_to_jobject<BluetoothGattCharacteristic>(env, array,
                                                                        "(J)V");
        return result;
    } catch (std::bad_alloc &e) {
        raise_java_oom_exception(env, e);
    } catch (BluetoothException &e) {
        raise_java_bluetooth_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_runtime_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_invalid_arg_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
    return nullptr;
}

void Java_tinyb_BluetoothGattService_delete(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
        delete obj_gatt_serv;
    } catch (std::bad_alloc &e) {
        raise_java_oom_exception(env, e);
    } catch (BluetoothException &e) {
        raise_java_bluetooth_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_runtime_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_invalid_arg_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
}

