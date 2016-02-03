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

#include "tinyb/BluetoothAdapter.hpp"
#include "tinyb/BluetoothDevice.hpp"
#include "tinyb/BluetoothObject.hpp"

#include "tinyb_BluetoothAdapter.h"

#include "helper.hpp"

using namespace tinyb;

jobject Java_tinyb_BluetoothAdapter_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "ADAPTER");
}

jobject Java_tinyb_BluetoothAdapter_clone(JNIEnv *env, jobject obj)
{
    return generic_clone<BluetoothAdapter>(env, obj);
}

jboolean Java_tinyb_BluetoothAdapter_startDiscovery(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return obj_adapter->start_discovery() ? JNI_TRUE : JNI_FALSE;
}

jboolean Java_tinyb_BluetoothAdapter_stopDiscovery(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return obj_adapter->stop_discovery() ? JNI_TRUE : JNI_FALSE;
}

jobject Java_tinyb_BluetoothAdapter_getDevices(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);
    std::vector<std::unique_ptr<BluetoothDevice>> array = obj_adapter->get_devices();
    jobject result = convert_vector_to_jobject<BluetoothDevice>(env, array,
                                                                "(J)V");

    return result;
}

jstring Java_tinyb_BluetoothAdapter_getAddress(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);
    std::string address = obj_adapter->get_address();

    return env->NewStringUTF((const char *)address.c_str());
}

jstring Java_tinyb_BluetoothAdapter_getName(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);
    std::string name = obj_adapter->get_name();

    return env->NewStringUTF((const char *)name.c_str());
}

jstring Java_tinyb_BluetoothAdapter_getAlias(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);
    std::string alias = obj_adapter->get_alias();

    return env->NewStringUTF((const char *)alias.c_str());
}

void Java_tinyb_BluetoothAdapter_setAlias(JNIEnv *env, jobject obj, jstring str)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    jboolean is_copy = JNI_TRUE;
    const char *str_chars = (char *)env->GetStringUTFChars(str, &is_copy);
    const std::string string_to_write = std::string(str_chars);

    env->ReleaseStringUTFChars(str, str_chars);

    obj_adapter->set_alias(string_to_write);
}

jlong Java_tinyb_BluetoothAdapter_getBluetoothClass(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return (jlong)obj_adapter->get_class();
}

jboolean Java_tinyb_BluetoothAdapter_getPowered(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return obj_adapter->get_powered() ? JNI_TRUE : JNI_FALSE;
}

void Java_tinyb_BluetoothAdapter_setPowered(JNIEnv *env, jobject obj, jboolean val)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    bool val_to_write = from_jboolean_to_bool(val);
    obj_adapter->set_powered(val_to_write);
}

jboolean Java_tinyb_BluetoothAdapter_getDiscoverable(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return obj_adapter->get_discoverable() ? JNI_TRUE : JNI_FALSE;
}

void Java_tinyb_BluetoothAdapter_setDiscoverable(JNIEnv *env, jobject obj, jboolean val)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    bool val_to_write = from_jboolean_to_bool(val);
    obj_adapter->set_discoverable(val_to_write);
}

jlong Java_tinyb_BluetoothAdapter_getDiscoverableTimeout(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return (jlong)obj_adapter->get_discoverable_timeout();
}

void Java_tinyb_BluetoothAdapter_setDiscoverableTimout(JNIEnv *env, jobject obj, jlong timeout)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    if (timeout < 0)
    {
        throw std::invalid_argument("timeout argument is negative\n");
    }
    obj_adapter->set_discoverable_timeout((unsigned int)timeout);
}

jboolean Java_tinyb_BluetoothAdapter_getPairable(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return obj_adapter->get_pairable() ? JNI_TRUE : JNI_FALSE;
}

void Java_tinyb_BluetoothAdapter_setPairable(JNIEnv *env, jobject obj, jboolean val)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    bool val_to_write = from_jboolean_to_bool(val);
    obj_adapter->set_pairable(val_to_write);
}

jlong Java_tinyb_BluetoothAdapter_getPairableTimeout(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return (jlong)obj_adapter->get_pairable_timeout();
}

void Java_tinyb_BluetoothAdapter_setPairableTimeout(JNIEnv *env, jobject obj, jlong timeout)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    if (timeout < 0)
    {
        throw std::invalid_argument("timeout argument is negative\n");
    }
    obj_adapter->set_pairable_timeout((unsigned int)timeout);
}

jboolean Java_tinyb_BluetoothAdapter_getDiscovering(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);

    return obj_adapter->get_discovering() ? JNI_TRUE : JNI_FALSE;
}

jobjectArray Java_tinyb_BluetoothAdapter_getUuids(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);
    std::vector<std::string> uuids = obj_adapter->get_uuids();
    unsigned int uuids_size = uuids.size();

    jclass string_class = search_class(env, "Ljava/lang/String;");
    jobjectArray result = env->NewObjectArray(uuids_size, string_class, 0);

    for (unsigned int i = 0; i < uuids_size; ++i)
    {
        std::string str_elem = uuids.at(i);
        jobject elem = env->NewStringUTF((const char *)str_elem.c_str());
        env->SetObjectArrayElement(result, i, elem);
    }

    return result;
}

jstring Java_tinyb_BluetoothAdapter_getModalias(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *obj_adapter = getInstance<BluetoothAdapter>(env, obj);
    std::string modalias = obj_adapter->get_modalias();

    return env->NewStringUTF((const char *)modalias.c_str());
}

void Java_tinyb_BluetoothAdapter_delete(JNIEnv *env, jobject obj)
{
    BluetoothAdapter *adapter = getInstance<BluetoothAdapter>(env, obj);
    delete adapter;
}

