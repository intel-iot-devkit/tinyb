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

#include "direct_bt_tinyb_Manager.h"

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/HCITypes.hpp"

using namespace direct_bt;

void Java_direct_1bt_tinyb_Manager_initImpl(JNIEnv *env, jobject obj)
{
    try {
        MgmtHandler *manager = &MgmtHandler::get(); // special: static singleton
        setInstance<MgmtHandler>(env, obj, manager);
        manager->setJavaObject( std::shared_ptr<JavaAnonObj>( new JavaGlobalObj(obj) ) );
        JavaGlobalObj::check(manager->getJavaObject(), E_FILE_LINE);
        DBG_PRINT("Java_direct_1bt_tinyb_Manager_init: Manager %s", manager->toString().c_str());
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

void Java_direct_1bt_tinyb_Manager_deleteImpl(JNIEnv *env, jobject obj)
{
    try {
        MgmtHandler *manager = getInstance<MgmtHandler>(env, obj); // special: static singleton
        manager->setJavaObject(nullptr);
        // delete manager;
        (void) manager;
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

static const std::string _adapterClazzCtorArgs("(JLjava/lang/String;Ljava/lang/String;)V");

jobject Java_direct_1bt_tinyb_Manager_getDefaultAdapterImpl(JNIEnv *env, jobject obj)
{
    try {
        MgmtHandler *manager = getInstance<MgmtHandler>(env, obj);
        DBG_PRINT("Java_direct_1bt_tinyb_Manager_getDefaultAdapterImpl: Manager %s", manager->toString().c_str());
        int defAdapterIdx = manager->getDefaultAdapterIdx();
        HCIAdapter * adapter = new HCIAdapter(defAdapterIdx);
        if( !adapter->isValid() ) {
            delete adapter;
            throw BluetoothException("Invalid default adapter "+std::to_string(defAdapterIdx), E_FILE_LINE);
        }
        if( !adapter->hasDevId() ) {
            delete adapter;
            throw BluetoothException("Invalid default adapter dev-id "+std::to_string(defAdapterIdx), E_FILE_LINE);
        }
        std::shared_ptr<direct_bt::HCISession> session = adapter->open();
        if( nullptr == session ) {
            delete adapter;
            throw BluetoothException("Couldn't open default adapter "+std::to_string(defAdapterIdx), E_FILE_LINE);
        }

        // prepare adapter ctor
        const jstring addr = from_string_to_jstring(env, adapter->getAddressString());
        const jstring name = from_string_to_jstring(env, adapter->getName());
        const jclass clazz = search_class(env, *adapter);
        if( nullptr == clazz ) {
            throw InternalError("Adapter class not found: "+HCIAdapter::java_class(), E_FILE_LINE);
        }
        const jmethodID clazz_ctor = search_method(env, clazz, "<init>", _adapterClazzCtorArgs.c_str(), false);
        if( nullptr == clazz_ctor ) {
            throw InternalError("Adapter ctor not found: "+HCIAdapter::java_class()+".<init>"+_adapterClazzCtorArgs, E_FILE_LINE);
        }
        exception_check_raise_and_throw(env, E_FILE_LINE);
        jobject jAdapter = env->NewObject(clazz, clazz_ctor, (jlong)adapter, addr, name);
        exception_check_raise_and_throw(env, E_FILE_LINE);
        JNIGlobalRef::check(jAdapter, E_FILE_LINE);
        std::shared_ptr<JavaAnonObj> jAdapterRef = adapter->getJavaObject();
        JavaGlobalObj::check(jAdapterRef, E_FILE_LINE);

        DBG_PRINT("Java_direct_1bt_tinyb_Manager_getDefaultAdapterImpl: New Adapter %s", adapter->toString().c_str());
        return JavaGlobalObj::GetObject(jAdapterRef);
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return NULL;
}

