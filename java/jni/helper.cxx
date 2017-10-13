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

#include <jni.h>
#include <memory>
#include <stdexcept>
#include <vector>

#include "helper.hpp"

jfieldID getInstanceField(JNIEnv *env, jobject obj)
{
    jclass clazz = env->GetObjectClass(obj);
    // J == long
    return env->GetFieldID(clazz, "nativeInstance", "J");
}

jclass search_class(JNIEnv *env, tinyb::BluetoothObject &object)
{
    return search_class(env, object.get_java_class().c_str());
}

jclass search_class(JNIEnv *env, const char *clazz_name)
{
    jclass clazz = env->FindClass(clazz_name);
    if (!clazz)
    {
        std::string error = "no class found: "; error += clazz_name;
        throw std::runtime_error(error);
    }
    return clazz;
}

jclass search_class(JNIEnv *env, jobject obj)
{
    jclass clazz = env->GetObjectClass(obj);
    if (!clazz)
    {
        std::string error = "no class found: ";
        throw std::runtime_error(error);
    }
    return clazz;
}

jmethodID search_method(JNIEnv *env, jclass clazz, const char *method_name,
                const char *prototype, bool is_static)
{
    jmethodID method;
    if (is_static)
    {
        method = env->GetStaticMethodID(clazz, method_name, prototype);
    }
    else
    {
        method = env->GetMethodID(clazz, method_name, prototype);
    }

    if (!method)
    {
        throw std::runtime_error("no method found\n");
    }

    return method;
}

jfieldID search_field(JNIEnv *env, jclass clazz, const char *field_name,
                const char *type, bool is_static)
{
    jfieldID field;
    if (is_static)
    {
        field = env->GetStaticFieldID(clazz, field_name, type);
    }
    else
    {
        field = env->GetFieldID(clazz, field_name, type);
    }

    if (!field)
    {
        throw std::runtime_error("no method found\n");
    }

    return field;
}

bool from_jboolean_to_bool(jboolean val)
{
    bool result;

    if (val == JNI_TRUE)
    {
        result = true;
    }
    else
    {
        if (val == JNI_FALSE)
        {
            result = false;
        }
        else
        {
            throw std::invalid_argument("the jboolean value is not true/false\n");
        }
    }

    return result;
}

std::string from_jstring_to_string(JNIEnv *env, jstring str)
{
    jboolean is_copy = JNI_TRUE;
    if (!str) {
        throw std::invalid_argument("String should not be null");
    }
    const char *str_chars = (char *)env->GetStringUTFChars(str, &is_copy);
    if (!str_chars) {
        throw std::bad_alloc();
    }
    const std::string string_to_write = std::string(str_chars);

    env->ReleaseStringUTFChars(str, str_chars);

    return string_to_write;
}

tinyb::BluetoothType from_int_to_btype(int type)
{
    tinyb::BluetoothType result = tinyb::BluetoothType::NONE;

    switch (type)
    {
        case 0:
            result = tinyb::BluetoothType::NONE;
            break;

        case 1:
            result = tinyb::BluetoothType::ADAPTER;
            break;

        case 2:
            result = tinyb::BluetoothType::DEVICE;
            break;

        case 3:
            result = tinyb::BluetoothType::GATT_SERVICE;
            break;

        case 4:
            result = tinyb::BluetoothType::GATT_CHARACTERISTIC;
            break;

        case 5:
            result = tinyb::BluetoothType::GATT_CHARACTERISTIC;
            break;

        default:
            result = tinyb::BluetoothType::NONE;
            break;
    }

    return result;
}

tinyb::TransportType from_int_to_transport_type(int type)
{
    tinyb::TransportType result = tinyb::TransportType::AUTO;

    switch (type)
    {
        case 0:
            result = tinyb::TransportType::AUTO;
            break;

        case 1:
            result = tinyb::TransportType::BREDR;
            break;

        case 2:
            result = tinyb::TransportType::LE;
            break;

        default:
            result = tinyb::TransportType::AUTO;
            break;
    }

    return result;
}

jobject get_bluetooth_type(JNIEnv *env, const char *field_name)
{
    jclass b_type_enum = search_class(env, JAVA_PACKAGE "/BluetoothType");

    jfieldID b_type_field = search_field(env, b_type_enum, field_name, "L" JAVA_PACKAGE "/BluetoothType;", true);

    jobject result = env->GetStaticObjectField(b_type_enum, b_type_field);
    env->DeleteLocalRef(b_type_enum);
    return result;
}

jobject get_new_arraylist(JNIEnv *env, unsigned int size, jmethodID *add)
{
    jclass arraylist_class = search_class(env, "Ljava/util/ArrayList;");
    jmethodID arraylist_ctor = search_method(env, arraylist_class, "<init>", "(I)V", false);

    jobject result = env->NewObject(arraylist_class, arraylist_ctor, size);
    if (!result)
    {
        throw std::runtime_error("cannot create instance of class\n");
    }

    *add = search_method(env, arraylist_class, "add", "(Ljava/lang/Object;)Z", false);

    env->DeleteLocalRef(arraylist_class);
    return result;
}

void raise_java_exception(JNIEnv *env, std::exception &e)
{
    env->ThrowNew(env->FindClass("java/lang/Error"), e.what());
}

void raise_java_runtime_exception(JNIEnv *env, std::runtime_error &e)
{
    env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
}

void raise_java_bluetooth_exception(JNIEnv *env, tinyb::BluetoothException &e)
{
    env->ThrowNew(env->FindClass("tinyb/BluetoothException"), e.what());
}


void raise_java_oom_exception(JNIEnv *env, std::bad_alloc &e)
{
    env->ThrowNew(env->FindClass("java/lang/OutOfMemoryException"), e.what());
}

void raise_java_invalid_arg_exception(JNIEnv *env, std::invalid_argument &e)
{
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), e.what());
}

