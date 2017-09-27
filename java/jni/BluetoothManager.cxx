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

#include "helper.hpp"

using namespace tinyb;

jobject Java_tinyb_BluetoothManager_getBluetoothType(JNIEnv *env, jobject obj)
{
    try {
        (void)obj;

        return get_bluetooth_type(env, "NONE");
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

static void getObject_setter(JNIEnv *env,
                                  jstring name, std::string **name_to_write,
                                  jstring identifier, std::string **identifier_to_write,
                                  jobject parent, BluetoothObject **b_parent)
{
    try {
        if (!parent)
        {
            *b_parent = nullptr;
        }
        else
        {
            *b_parent = getInstance<BluetoothObject>(env, parent);
        }

        if (!name)
        {
            *name_to_write = nullptr;
        }
        else
        {
            *name_to_write = new std::string(from_jstring_to_string(env, name));
        }

        if (!identifier)
        {
            *identifier_to_write = nullptr;
        }
        else
        {
            *identifier_to_write = new std::string(from_jstring_to_string(env, identifier));
        }
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

static void getObject_cleaner(std::string *name_to_write, std::string *identifier_to_write)
{
    if (name_to_write != nullptr)
        delete name_to_write;

    if (identifier_to_write != nullptr)
        delete identifier_to_write;
}

jobject Java_tinyb_BluetoothManager_find(JNIEnv *env, jobject obj, jint type,
                                            jstring name, jstring identifier, jobject parent,
                                            jlong milliseconds)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        BluetoothObject *b_parent;
        BluetoothType b_type;
        std::string *name_to_write;
        std::string *identifier_to_write;

        getObject_setter(env,
                         name, &name_to_write,
                         identifier, &identifier_to_write,
                         parent, &b_parent);

        b_type = from_int_to_btype((int)type);
        std::unique_ptr<BluetoothObject> b_object = manager->find(b_type, name_to_write,
                                                                identifier_to_write,
                                                                b_parent,
                                                                std::chrono::milliseconds(milliseconds));
        getObject_cleaner(name_to_write, identifier_to_write);

        BluetoothObject *b_object_naked = b_object.release();
        if (!b_object_naked)
        {
            return nullptr;
        }
        jclass clazz = search_class(env, *b_object_naked);
        jmethodID clazz_ctor = search_method(env, clazz, "<init>", "(J)V", false);

        jobject result = env->NewObject(clazz, clazz_ctor, (jlong)b_object_naked);

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


jobject Java_tinyb_BluetoothManager_getObject(JNIEnv *env, jobject obj, jint type,
                                            jstring name, jstring identifier, jobject parent)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        BluetoothObject *b_parent;
        BluetoothType b_type;
        std::string *name_to_write;
        std::string *identifier_to_write;

        getObject_setter(env,
                         name, &name_to_write,
                         identifier, &identifier_to_write,
                         parent, &b_parent);

        b_type = from_int_to_btype((int)type);
        std::unique_ptr<BluetoothObject> b_object = manager->get_object(b_type, name_to_write,
                                                                identifier_to_write,
                                                                b_parent);
        getObject_cleaner(name_to_write, identifier_to_write);

        BluetoothObject *b_object_naked = b_object.release();
        if (!b_object_naked)
        {
            return nullptr;
        }
        jclass clazz = search_class(env, *b_object_naked);
        jmethodID clazz_ctor = search_method(env, clazz, "<init>", "(J)V", false);

        jobject result = env->NewObject(clazz, clazz_ctor, (jlong)b_object_naked);
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

jobject Java_tinyb_BluetoothManager_getObjects(JNIEnv *env, jobject obj, jint type,
                                            jstring name, jstring identifier, jobject parent)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        BluetoothObject *b_parent;
        BluetoothType b_type;
        std::string *name_to_write;
        std::string *identifier_to_write;

        getObject_setter(env,
                         name, &name_to_write,
                         identifier, &identifier_to_write,
                         parent, &b_parent);

        b_type = from_int_to_btype((int)type);
        std::vector<std::unique_ptr<BluetoothObject>> array = manager->get_objects(b_type,
                                                                    name_to_write,
                                                                    identifier_to_write,
                                                                    b_parent);
        getObject_cleaner(name_to_write, identifier_to_write);
        jobject result = convert_vector_to_jobject<BluetoothObject>(env, array, "(J)V");
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

jobject Java_tinyb_BluetoothManager_getAdapters(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

        std::vector<std::unique_ptr<BluetoothAdapter>> array = manager->get_adapters();
        jobject result = convert_vector_to_jobject<BluetoothAdapter>(env, array,
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

jobject Java_tinyb_BluetoothManager_getDevices(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

        std::vector<std::unique_ptr<BluetoothDevice>> array = manager->get_devices();
        jobject result = convert_vector_to_jobject<BluetoothDevice>(env, array,
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

jobject Java_tinyb_BluetoothManager_getServices(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

        std::vector<std::unique_ptr<BluetoothGattService>> array = manager->get_services();
        jobject result = convert_vector_to_jobject<BluetoothGattService>(env, array,
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

jboolean Java_tinyb_BluetoothManager_setDefaultAdapter(JNIEnv *env, jobject obj, jobject adapter)
{
    try {
        if (adapter == nullptr)
            throw std::invalid_argument("adapter argument is null\n");

        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        BluetoothAdapter *b_adapter = getInstance<BluetoothAdapter>(env, adapter);

        return manager->set_default_adapter(*b_adapter);
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

jboolean Java_tinyb_BluetoothManager_startDiscovery(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        return manager->start_discovery() ? JNI_TRUE : JNI_FALSE;
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

jboolean Java_tinyb_BluetoothManager_stopDiscovery(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        return manager->start_discovery() ? JNI_TRUE : JNI_FALSE;
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

jboolean Java_tinyb_BluetoothManager_getDiscovering(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        return manager->get_discovering() ? JNI_TRUE : JNI_FALSE;
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

void Java_tinyb_BluetoothManager_init(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();
        setInstance<BluetoothManager>(env, obj, manager);
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

void Java_tinyb_BluetoothManager_delete(JNIEnv *env, jobject obj)
{
    try {
        BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
        delete manager;
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

jstring Java_tinyb_BluetoothManager_getNativeAPIVersion(JNIEnv *env, jclass clazz)
{
    try {
        (void) clazz;

        BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();
        return env->NewStringUTF(manager->get_api_version().c_str());
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
