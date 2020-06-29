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

#include "direct_bt_tinyb_DBTManager.h"

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTManager.hpp"
#include "direct_bt/DBTDevice.hpp"
#include "direct_bt/DBTAdapter.hpp"

using namespace direct_bt;

void Java_direct_1bt_tinyb_DBTManager_initImpl(JNIEnv *env, jobject obj, jboolean unifyUUID128Bit)
{
    directBTJNISettings.setUnifyUUID128Bit(unifyUUID128Bit);
    try {
        DBTManager *manager = &DBTManager::get(BTMode::BT_MODE_LE); // special: static singleton
        setInstance<DBTManager>(env, obj, manager);
        java_exception_check_and_throw(env, E_FILE_LINE);
        manager->setJavaObject( std::shared_ptr<JavaAnonObj>( new JavaGlobalObj(obj, nullptr) ) );
        JavaGlobalObj::check(manager->getJavaObject(), E_FILE_LINE);
        DBG_PRINT("Java_direct_1bt_tinyb_DBTManager_init: Manager %s", manager->toString().c_str());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

void Java_direct_1bt_tinyb_DBTManager_deleteImpl(JNIEnv *env, jobject obj, jlong nativeInstance)
{
    (void)obj;
    try {
        DBTManager *manager = castInstance<DBTManager>(nativeInstance); // special: static singleton
        manager->close();
        manager->setJavaObject(nullptr);
        (void) manager;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

static const std::string _adapterClazzCtorArgs("(JLjava/lang/String;Ljava/lang/String;)V");

jobject Java_direct_1bt_tinyb_DBTManager_getAdapterListImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTManager *manager = getInstance<DBTManager>(env, obj);
        DBG_PRINT("Java_direct_1bt_tinyb_DBTManager_getAdapterListImpl: Manager %s", manager->toString().c_str());

        // index == dev_id
        std::vector<std::unique_ptr<DBTAdapter>> adapters;
        const int adapterCount = manager->getAdapterCount();
        for(int idx = 0; idx < adapterCount; idx++) {
            std::unique_ptr<DBTAdapter> adapter(new DBTAdapter( idx ) );
            if( !adapter->isValid() ) {
                throw BluetoothException("Invalid adapter @ idx "+std::to_string( idx ), E_FILE_LINE);
            }
            if( !adapter->hasDevId() ) {
                throw BluetoothException("Invalid adapter dev-id @ idx "+std::to_string( idx ), E_FILE_LINE);
            }
            if( idx != adapter->dev_id ) { // just make sure idx == dev_id
                throw BluetoothException("Invalid adapter dev-id "+std::to_string( adapter->dev_id )+" != index "+std::to_string( idx ), E_FILE_LINE);
            }
            adapters.push_back(std::move(adapter));
        }
        std::function<jobject(JNIEnv*, jclass, jmethodID, DBTAdapter*)> ctor_adapter =
                [](JNIEnv *env, jclass clazz, jmethodID clazz_ctor, DBTAdapter* adapter)->jobject {
                    // prepare adapter ctor
                    const jstring addr = from_string_to_jstring(env, adapter->getAddressString());
                    const jstring name = from_string_to_jstring(env, adapter->getName());
                    java_exception_check_and_throw(env, E_FILE_LINE);
                    jobject jAdapter = env->NewObject(clazz, clazz_ctor, (jlong)adapter, addr, name);
                    java_exception_check_and_throw(env, E_FILE_LINE);
                    JNIGlobalRef::check(jAdapter, E_FILE_LINE);
                    std::shared_ptr<JavaAnonObj> jAdapterRef = adapter->getJavaObject();
                    JavaGlobalObj::check(jAdapterRef, E_FILE_LINE);

                    DBG_PRINT("Java_direct_1bt_tinyb_DBTManager_getAdapterListImpl: New Adapter %p %s", adapter, adapter->toString().c_str());
                    return JavaGlobalObj::GetObject(jAdapterRef);
                };
        return convert_vector_uniqueptr_to_jarraylist<DBTAdapter>(env, adapters, _adapterClazzCtorArgs.c_str(), ctor_adapter);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

