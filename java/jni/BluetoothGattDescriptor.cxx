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

#include "JNIMem.hpp"
#include "helper.hpp"

using namespace tinyb;

jobject Java_tinyb_BluetoothGattDescriptor_getBluetoothType(JNIEnv *env, jobject obj)
{
    try {
        (void)obj;

        return get_bluetooth_type(env, "GATT_DESCRIPTOR");
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

jobject Java_tinyb_BluetoothGattDescriptor_clone(JNIEnv *env, jobject obj)
{
    try {
        return generic_clone<BluetoothGattDescriptor>(env, obj);
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

jbyteArray Java_tinyb_BluetoothGattDescriptor_readValue(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
        std::vector<unsigned char> array = obj_gatt_desc->read_value();
        unsigned int array_size = array.size();

        jbyteArray result = env->NewByteArray((jsize)array_size);
        env->SetByteArrayRegion(result, 0, (jsize)array_size, (const jbyte *)&array[0]);

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

jboolean Java_tinyb_BluetoothGattDescriptor_writeValue(JNIEnv *env, jobject obj, jbyteArray argValue)
{
    try {
        if (!argValue)
        {
            throw std::invalid_argument("byte array is null");
        }

        BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);

        jboolean is_copy = false;
        jbyte *native_array = env->GetByteArrayElements(argValue, &is_copy);
        jsize native_array_length = env->GetArrayLength(argValue);

        std::vector<unsigned char> array(native_array, native_array + native_array_length);

        return obj_gatt_desc->write_value(array) ? JNI_TRUE : JNI_FALSE;
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

jstring Java_tinyb_BluetoothGattDescriptor_getUUID(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
        std::string uuid = obj_gatt_desc->get_uuid();

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

jobject Java_tinyb_BluetoothGattDescriptor_getCharacteristic(JNIEnv *env, jobject obj)
{
    try {
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

jbyteArray Java_tinyb_BluetoothGattDescriptor_getValue(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
        std::vector<unsigned char> array = obj_gatt_desc->get_value();
        unsigned int array_size = array.size();

        jbyteArray result = env->NewByteArray((jsize)array_size);
        env->SetByteArrayRegion(result, 0, (jsize)array_size, (const jbyte *)&array[0]);

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

void Java_tinyb_BluetoothGattDescriptor_enableValueNotifications(JNIEnv *env, jobject obj, jobject callback)
{
    try {
        BluetoothGattDescriptor *obj_gatt_desc =
                                    getInstance<BluetoothGattDescriptor>(env, obj);
        std::shared_ptr<JNIGlobalRef> callback_ptr(new JNIGlobalRef(callback));
        obj_gatt_desc->enable_value_notifications([ callback_ptr ] (std::vector<unsigned char> &v)
            {
                jclass notification = search_class(*jni_env, **callback_ptr);
                jmethodID  method = search_method(*jni_env, notification, "run", "(Ljava/lang/Object;)V", false);
                jni_env->DeleteLocalRef(notification);
                unsigned int size = v.size();

                jbyteArray result = jni_env->NewByteArray((jsize)size);
                jni_env->SetByteArrayRegion(result, 0, (jsize)size, (const jbyte *)&v[0]);

                jni_env->CallVoidMethod(**callback_ptr, method, result);
                jni_env->DeleteLocalRef(result);

            });
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

void Java_tinyb_BluetoothGattDescriptor_disableValueNotifications(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattDescriptor *obj_gatt_desc =
                                    getInstance<BluetoothGattDescriptor>(env, obj);
        obj_gatt_desc->disable_value_notifications();
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


void Java_tinyb_BluetoothGattDescriptor_delete(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
        delete obj_gatt_desc;
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

