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

#include "direct_bt_tinyb_Adapter.h"

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/HCITypes.hpp"

using namespace direct_bt;

static const std::string _deviceClazzCtorArgs("(JLdirect_bt/tinyb/Adapter;Ljava/lang/String;Ljava/lang/String;J)V");
static const std::string _deviceDiscoveryMethodArgs("(Lorg/tinyb/BluetoothAdapter;Lorg/tinyb/BluetoothDevice;)V");

class DeviceDiscoveryCallbackListener : public HCIDeviceDiscoveryListener {
  public:
    /**
        package org.tinyb;

        public interface BluetoothDeviceDiscoveryListener {
            public void deviceAdded(final BluetoothAdapter adapter, final BluetoothDevice device);
            public void deviceUpdated(final BluetoothAdapter adapter, final BluetoothDevice device);
            public void deviceRemoved(final BluetoothAdapter adapter, final BluetoothDevice device);
        };
     */
    std::shared_ptr<JavaAnonObj> adapterObjRef;
    std::unique_ptr<JNIGlobalRef> deviceClazzRef;
    jmethodID deviceClazzCtor;
    jfieldID deviceClazzTSUpdateField;
    std::unique_ptr<JNIGlobalRef> listenerObjRef;
    std::unique_ptr<JNIGlobalRef> listenerClazzRef;
    jmethodID  mDeviceAdded = nullptr;
    jmethodID  mDeviceUpdated = nullptr;
    jmethodID  mDeviceRemoved = nullptr;

    DeviceDiscoveryCallbackListener(JNIEnv *env, HCIAdapter *adapter, jobject deviceDiscoveryListener) {
        adapterObjRef = adapter->getJavaObject();
        JavaGlobalObj::check(adapterObjRef, E_FILE_LINE);
        {
            jclass deviceClazz = search_class(*jni_env, HCIDevice::java_class().c_str());
            if( nullptr == deviceClazz ) {
                throw InternalError("HCIDevice::java_class not found: "+HCIDevice::java_class(), E_FILE_LINE);
            }
            deviceClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(deviceClazz));
            env->DeleteLocalRef(deviceClazz);
        }
        deviceClazzCtor = search_method(*jni_env, deviceClazzRef->getClass(), "<init>", _deviceClazzCtorArgs.c_str(), false);
        if( nullptr == deviceClazzCtor ) {
            throw InternalError("HCIDevice::java_class ctor not found: "+HCIDevice::java_class()+".<init>"+_deviceClazzCtorArgs, E_FILE_LINE);
        }
        deviceClazzTSUpdateField = jni_env->GetFieldID(deviceClazzRef->getClass(), "ts_update", "J");
        if( nullptr == deviceClazzTSUpdateField ) {
            throw InternalError("HCIDevice::java_class field not found: "+HCIDevice::java_class()+".ts_update", E_FILE_LINE);
        }

        listenerObjRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(deviceDiscoveryListener));
        {
            jclass listenerClazz = search_class(env, listenerObjRef->getObject());
            if( nullptr == listenerClazz ) {
                throw InternalError("BluetoothDeviceDiscoveryListener not found", E_FILE_LINE);
            }
            listenerClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(listenerClazz));
            env->DeleteLocalRef(listenerClazz);
        }
        mDeviceAdded = search_method(env, listenerClazzRef->getClass(), "deviceAdded", _deviceDiscoveryMethodArgs.c_str(), false);
        mDeviceUpdated = search_method(env, listenerClazzRef->getClass(), "deviceUpdated", _deviceDiscoveryMethodArgs.c_str(), false);
        mDeviceRemoved = search_method(env, listenerClazzRef->getClass(), "deviceRemoved", _deviceDiscoveryMethodArgs.c_str(), false);
        if( nullptr == mDeviceAdded ) {
            throw InternalError("BluetoothDeviceDiscoveryListener has no deviceAdded"+_deviceDiscoveryMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        if( nullptr == mDeviceUpdated ) {
            throw InternalError("BluetoothDeviceDiscoveryListener has no deviceUpdated"+_deviceDiscoveryMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        if( nullptr == mDeviceRemoved ) {
            throw InternalError("BluetoothDeviceDiscoveryListener has no deviceRemoved"+_deviceDiscoveryMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        exception_check_raise_and_throw(env, E_FILE_LINE);
    }

    void deviceAdded(HCIAdapter const &a, std::shared_ptr<HCIDevice> device) override {
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** Native Adapter Device ADDED__: %s\n", device->toString().c_str());
                fprintf(stderr, "Status HCIAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif

            // Device(final long nativeInstance, final Adapter adptr, final String address, final String name)
            const jstring addr = from_string_to_jstring(*jni_env, device->getAddressString());
            const jstring name = from_string_to_jstring(*jni_env, device->getName());
            jobject jDevice = jni_env->NewObject(deviceClazzRef->getClass(), deviceClazzCtor,
                    (jlong)device.get(), adapterObjRef, addr, name, (jlong)device->ts_creation);
            exception_check_raise_and_throw(*jni_env, E_FILE_LINE);
            JNIGlobalRef::check(jDevice, E_FILE_LINE);
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);

            jni_env->CallVoidMethod(listenerObjRef->getObject(), mDeviceAdded, JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef));
            exception_check_raise_and_throw(*jni_env, E_FILE_LINE);
        } CATCH_EXCEPTION_AND_RAISE_JAVA(*jni_env, e)
    }
    void deviceUpdated(HCIAdapter const &a, std::shared_ptr<HCIDevice> device) override {
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** Native Adapter Device UPDATED: %s\n", device->toString().c_str());
                fprintf(stderr, "Status HCIAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);
            jni_env->SetLongField(JavaGlobalObj::GetObject(jDeviceRef), deviceClazzTSUpdateField, (jlong)device->getUpdateTimestamp());
            jni_env->CallVoidMethod(listenerObjRef->getObject(), mDeviceUpdated, JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef));
            exception_check_raise_and_throw(*jni_env, E_FILE_LINE);
        } CATCH_EXCEPTION_AND_RAISE_JAVA(*jni_env, e)
    }
    void deviceRemoved(HCIAdapter const &a, std::shared_ptr<HCIDevice> device) override {
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** DBTAdapter Device REMOVED: %s\n", device->toString().c_str());
                fprintf(stderr, "Status HCIAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);
            jni_env->SetLongField(JavaGlobalObj::GetObject(jDeviceRef), deviceClazzTSUpdateField, (jlong)device->getUpdateTimestamp());
            jni_env->CallVoidMethod(listenerObjRef->getObject(), mDeviceRemoved, JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef));
            exception_check_raise_and_throw(*jni_env, E_FILE_LINE);
        } CATCH_EXCEPTION_AND_RAISE_JAVA(*jni_env, e)
    }
};

void Java_direct_1bt_tinyb_Adapter_initImpl(JNIEnv *env, jobject obj, jobject deviceDiscoveryListener)
{
    // org.tinyb.BluetoothDeviceDiscoveryListener
    try {
        HCIAdapter *adapter = getInstance<HCIAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        // set our callback discovery listener.
        DeviceDiscoveryCallbackListener *l = new DeviceDiscoveryCallbackListener(env, adapter, deviceDiscoveryListener);
        adapter->setDeviceDiscoveryListener(std::shared_ptr<HCIDeviceDiscoveryListener>(l));
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

void Java_direct_1bt_tinyb_Adapter_deleteImpl(JNIEnv *env, jobject obj)
{
    try {
        HCIAdapter *adapter = getInstance<HCIAdapter>(env, obj);
        DBG_PRINT("Java_direct_1bt_tinyb_Adapter_deleteImpl %s", adapter->toString().c_str());
        delete adapter;
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

jboolean Java_direct_1bt_tinyb_Adapter_startDiscoveryImpl(JNIEnv *env, jobject obj)
{
    try {
        HCIAdapter *adapter = getInstance<HCIAdapter>(env, obj);
        std::shared_ptr<direct_bt::HCISession> session = adapter->getOpenSession();
        if( nullptr == session ) {
            throw BluetoothException("No adapter session: "+adapter->toString(), E_FILE_LINE);
        }
        if( !session->isOpen() ) {
            throw BluetoothException("No open adapter session: "+adapter->toString(), E_FILE_LINE);
        }
        return adapter->startDiscovery(*session);
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_Adapter_stopDiscoveryImpl(JNIEnv *env, jobject obj)
{
    try {
        HCIAdapter *adapter = getInstance<HCIAdapter>(env, obj);
        std::shared_ptr<direct_bt::HCISession> session = adapter->getOpenSession();
        if( nullptr == session ) {
            throw BluetoothException("No adapter session: "+adapter->toString(), E_FILE_LINE);
        }
        if( !session->isOpen() ) {
            throw BluetoothException("No open adapter session: "+adapter->toString(), E_FILE_LINE);
        }
        adapter->setDeviceDiscoveryListener(nullptr);
        adapter->stopDiscovery(*session);
        return JNI_TRUE;
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return JNI_FALSE;
}

jint Java_direct_1bt_tinyb_Adapter_discoverAnyDeviceImpl(JNIEnv *env, jobject obj, jint timeoutMS)
{
    try {
        HCIAdapter *adapter = getInstance<HCIAdapter>(env, obj);
        std::shared_ptr<direct_bt::HCISession> session = adapter->getOpenSession();
        if( nullptr == session ) {
            throw BluetoothException("No adapter session: "+adapter->toString(), E_FILE_LINE);
        }
        if( !session->isOpen() ) {
            throw BluetoothException("No open adapter session: "+adapter->toString(), E_FILE_LINE);
        }
        return adapter->discoverDevices(*session, -1, EUI48_ANY_DEVICE, timeoutMS, static_cast<uint32_t>(EInfoReport::Element::NAME));
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return 0;
}

jobject Java_direct_1bt_tinyb_Adapter_getDiscoveredDevicesImpl(JNIEnv *env, jobject obj)
{
    try {
        HCIAdapter *adapter = getInstance<HCIAdapter>(env, obj);
        /**
        std::function<jobject(JNIEnv*, jclass, jmethodID, HCIDevice*)> ctor_device = [&](JNIEnv *env, jclass clazz, jmethodID clazz_ctor, HCIDevice *elem) {
            // Device(final long nativeInstance, final Adapter adptr, final String address, final String name)
            jstring addr = from_string_to_jstring(env, elem->getAddressString());
            jstring name = from_string_to_jstring(env, elem->getName());
            jobject object = env->NewObject(clazz, clazz_ctor, (jlong)elem, obj, addr, name);
            return object;
        };
        return convert_vector_to_jobject<HCIDevice>(env, array, "(JLdirect_bt/tinyb/Adapter;Ljava/lang/String;Ljava/lang/String;)V", ctor_device);
        */
        std::vector<std::shared_ptr<HCIDevice>> array = adapter->getDiscoveredDevices();
        return convert_vector_to_jobject(env, array);
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return nullptr;
}

jint Java_direct_1bt_tinyb_Adapter_removeDevicesImpl(JNIEnv *env, jobject obj)
{
    try {
        HCIAdapter *adapter = getInstance<HCIAdapter>(env, obj);
        return adapter->removeDiscoveredDevices();
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return 0;
}

