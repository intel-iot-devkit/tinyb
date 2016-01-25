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
#include "tinyb/BluetoothGattService.hpp"
#include "tinyb/BluetoothManager.hpp"

#include "tinyb_BluetoothManager.h"

#include "helper.h"

using namespace tinyb;

jobject Java_tinyb_BluetoothManager_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "NONE");
}

jobject Java_tinyb_BluetoothManager_getObject(JNIEnv *env, jobject obj, jobject type,
                                        jstring name, jstring identifier, jobject parent)
{
    (void)env;
    (void)obj;
    (void)type;
    (void)name;
    (void)identifier;
    (void)parent;

    return nullptr;
}

jobject Java_tinyb_BluetoothManager_getObjects(JNIEnv *env, jobject obj, jobject type,
                                        jstring name, jstring identifier, jobject parent)
{
    (void)env;
    (void)obj;
    (void)type;
    (void)name;
    (void)identifier;
    (void)parent;

    return nullptr;
}

jobject Java_tinyb_BluetoothManager_getAdapters(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

    std::vector<std::unique_ptr<BluetoothAdapter>> array = manager->get_adapters();
    jobject result = convert_vector_to_jobject<BluetoothAdapter>(env, array,
                                                                "BluetoothAdapter",
                                                                "(J)V");
    return result;
}

jobject Java_tinyb_BluetoothManager_getDevices(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

    std::vector<std::unique_ptr<BluetoothDevice>> array = manager->get_devices();
    jobject result = convert_vector_to_jobject<BluetoothDevice>(env, array,
                                                                "BluetoothDevice",
                                                                "(J)V");
    return result;

}

jobject Java_tinyb_BluetoothManager_getServices(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

    std::vector<std::unique_ptr<BluetoothGattService>> array = manager->get_services();
    jobject result = convert_vector_to_jobject<BluetoothGattService>(env, array,
                                                                "BluetoothGattService",
                                                                "(J)V");
    return result;
}

jboolean Java_tinyb_BluetoothManager_setDefaultAdapter(JNIEnv *env, jobject obj, jobject adapter)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    BluetoothAdapter *b_adapter = getInstance<BluetoothAdapter>(env, adapter);

    return manager->set_default_adapter(b_adapter);
}

jboolean Java_tinyb_BluetoothManager_startDiscovery(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    return manager->start_discovery() ? JNI_TRUE : JNI_FALSE;
}

jboolean Java_tinyb_BluetoothManager_stopDiscovery(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    return manager->start_discovery() ? JNI_TRUE : JNI_FALSE;
}

void Java_tinyb_BluetoothManager_init(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();
    setInstance<BluetoothManager>(env, obj, manager);
}

void Java_tinyb_BluetoothManager_delete(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    delete manager;
}

