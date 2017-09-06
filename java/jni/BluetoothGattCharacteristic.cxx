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
#include "tinyb/BluetoothGattService.hpp"
#include "tinyb/BluetoothObject.hpp"

#include "tinyb_BluetoothGattCharacteristic.h"

#include "JNIMem.hpp"
#include "helper.hpp"

using namespace tinyb;

jobject Java_tinyb_BluetoothGattCharacteristic_getBluetoothType(JNIEnv *env, jobject obj)
{
    try {
        (void)obj;

        return get_bluetooth_type(env, "GATT_CHARACTERISTIC");
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

jobject Java_tinyb_BluetoothGattCharacteristic_clone(JNIEnv *env, jobject obj)
{
    try {
        return generic_clone<BluetoothGattCharacteristic>(env, obj);
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

jbyteArray Java_tinyb_BluetoothGattCharacteristic_readValue(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        std::vector<unsigned char> array = obj_gatt_char->read_value();
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

jboolean Java_tinyb_BluetoothGattCharacteristic_writeValue(JNIEnv *env, jobject obj, jbyteArray argValue)
{
    try {
        if (!argValue)
        {
            throw std::invalid_argument("byte array is null");
            return JNI_FALSE;
        }

        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);

        jboolean is_copy = false;
        jbyte *native_array = env->GetByteArrayElements(argValue, &is_copy);
        jsize native_array_length = env->GetArrayLength(argValue);

        std::vector<unsigned char> array(native_array, native_array + native_array_length);

        return obj_gatt_char->write_value(array) ? JNI_TRUE : JNI_FALSE;
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

void Java_tinyb_BluetoothGattCharacteristic_enableValueNotifications(JNIEnv *env, jobject obj, jobject callback)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        std::shared_ptr<JNIGlobalRef> callback_ptr(new JNIGlobalRef(callback));
        obj_gatt_char->enable_value_notifications([ callback_ptr ] (std::vector<unsigned char> &v)
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

void Java_tinyb_BluetoothGattCharacteristic_disableValueNotifications(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        obj_gatt_char->disable_value_notifications();
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

jstring Java_tinyb_BluetoothGattCharacteristic_getUUID(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        std::string uuid = obj_gatt_char->get_uuid();

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

jobject Java_tinyb_BluetoothGattCharacteristic_getService(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        BluetoothGattService *obj_gatt_serv = obj_gatt_char->get_service().clone();

        jclass b_gatt_serv_class = search_class(env, *obj_gatt_serv);
        jmethodID b_gatt_serv_ctor = search_method(env, b_gatt_serv_class, "<init>",
                                                "(J)V", false);
        jobject result = env->NewObject(b_gatt_serv_class, b_gatt_serv_ctor, (jlong)obj_gatt_serv);
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

jbyteArray Java_tinyb_BluetoothGattCharacteristic_getValue(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        std::vector<unsigned char> array = obj_gatt_char->get_value();
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

jboolean Java_tinyb_BluetoothGattCharacteristic_getNotifying(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        return obj_gatt_char->get_notifying() ? JNI_TRUE : JNI_FALSE;
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

jobjectArray Java_tinyb_BluetoothGattCharacteristic_getFlags(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        std::vector<std::string> flags = obj_gatt_char->get_flags();
        unsigned int flags_size = flags.size();

        jclass string_class = search_class(env, "Ljava/lang/String;");
        jobjectArray result = env->NewObjectArray(flags_size, string_class, 0);

        for (unsigned int i = 0; i < flags_size; ++i)
        {
            std::string str_elem = flags.at(i);
            jobject elem = env->NewStringUTF((const char *)str_elem.c_str());
            env->SetObjectArrayElement(result, i, elem);
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

jobject Java_tinyb_BluetoothGattCharacteristic_getDescriptors(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        std::vector<std::unique_ptr<BluetoothGattDescriptor>> array = obj_gatt_char->get_descriptors();

        jobject result = convert_vector_to_jobject<BluetoothGattDescriptor>(env, array,
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

void Java_tinyb_BluetoothGattCharacteristic_delete(JNIEnv *env, jobject obj)
{
    try {
        BluetoothGattCharacteristic *obj_gatt_char =
                                    getInstance<BluetoothGattCharacteristic>(env, obj);
        delete obj_gatt_char;
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
