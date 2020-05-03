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

#include "direct_bt_tinyb_DBTAdapter.h"

#define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTTypes.hpp"

using namespace direct_bt;

static const std::string _eirDataTypeClassName("org/tinyb/EIRDataType");
static const std::string _eirDataTypeClazzCreateArgs("(I)Lorg/tinyb/EIRDataType;");
static const std::string _deviceClazzCtorArgs("(JLdirect_bt/tinyb/DBTAdapter;Ljava/lang/String;Ljava/lang/String;J)V");
static const std::string _deviceStatusMethodArgs("(Lorg/tinyb/BluetoothAdapter;Lorg/tinyb/BluetoothDevice;J)V");
static const std::string _deviceStatusUpdateMethodArgs("(Lorg/tinyb/BluetoothAdapter;Lorg/tinyb/BluetoothDevice;JLorg/tinyb/EIRDataType;)V");

class DeviceStatusCallbackListener : public DBTDeviceStatusListener {
  public:
    /**
        package org.tinyb;

        public interface BluetoothDeviceStatusListener {
            public void deviceFound(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
            public void deviceUpdated(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp, final EIRDataType updateMask);
            public void deviceConnected(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
            public void deviceDisconnected(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
        };
     */
    std::shared_ptr<JavaAnonObj> adapterObjRef;
    std::unique_ptr<JNIGlobalRef> eirDataTypeClazzRef;
    jmethodID eirDataTypeClazzCreate;
    std::unique_ptr<JNIGlobalRef> deviceClazzRef;
    jmethodID deviceClazzCtor;
    jfieldID deviceClazzTSUpdateField;
    std::unique_ptr<JNIGlobalRef> listenerObjRef;
    std::unique_ptr<JNIGlobalRef> listenerClazzRef;
    jmethodID  mDeviceFound = nullptr;
    jmethodID  mDeviceUpdated = nullptr;
    jmethodID  mDeviceConnected = nullptr;
    jmethodID  mDeviceDisconnected = nullptr;

    DeviceStatusCallbackListener(JNIEnv *env, DBTAdapter *adapter, jobject deviceDiscoveryListener) {
        adapterObjRef = adapter->getJavaObject();
        JavaGlobalObj::check(adapterObjRef, E_FILE_LINE);

        // eirDataTypeClazzRef, eirDataTypeClazzCreate
        {
            jclass eirDataTypeClazz = search_class(env, _eirDataTypeClassName.c_str());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == eirDataTypeClazz ) {
                throw InternalError("DBTDevice::java_class not found: "+_eirDataTypeClassName, E_FILE_LINE);
            }
            eirDataTypeClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(eirDataTypeClazz));
            env->DeleteLocalRef(eirDataTypeClazz);
        }
        eirDataTypeClazzCreate = search_method(env, eirDataTypeClazzRef->getClass(), "create", _eirDataTypeClazzCreateArgs.c_str(), true);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == eirDataTypeClazzCreate ) {
            throw InternalError("EIRDataType ctor not found: "+_eirDataTypeClassName+".create"+_eirDataTypeClazzCreateArgs, E_FILE_LINE);
        }

        // deviceClazzRef, deviceClazzCtor
        {
            jclass deviceClazz = search_class(env, DBTDevice::java_class().c_str());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == deviceClazz ) {
                throw InternalError("DBTDevice::java_class not found: "+DBTDevice::java_class(), E_FILE_LINE);
            }
            deviceClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(deviceClazz));
            env->DeleteLocalRef(deviceClazz);
        }
        deviceClazzCtor = search_method(env, deviceClazzRef->getClass(), "<init>", _deviceClazzCtorArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == deviceClazzCtor ) {
            throw InternalError("DBTDevice::java_class ctor not found: "+DBTDevice::java_class()+".<init>"+_deviceClazzCtorArgs, E_FILE_LINE);
        }
        deviceClazzTSUpdateField = env->GetFieldID(deviceClazzRef->getClass(), "ts_update", "J");
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == deviceClazzTSUpdateField ) {
            throw InternalError("DBTDevice::java_class field not found: "+DBTDevice::java_class()+".ts_update", E_FILE_LINE);
        }

        listenerObjRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(deviceDiscoveryListener));
        {
            jclass listenerClazz = search_class(env, listenerObjRef->getObject());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == listenerClazz ) {
                throw InternalError("BluetoothDeviceDiscoveryListener not found", E_FILE_LINE);
            }
            listenerClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(listenerClazz));
            env->DeleteLocalRef(listenerClazz);
        }
        mDeviceFound = search_method(env, listenerClazzRef->getClass(), "deviceFound", _deviceStatusMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceFound ) {
            throw InternalError("BluetoothDeviceDiscoveryListener has no deviceFound"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceUpdated = search_method(env, listenerClazzRef->getClass(), "deviceUpdated", _deviceStatusUpdateMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceUpdated ) {
            throw InternalError("BluetoothDeviceDiscoveryListener has no deviceUpdated"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceConnected = search_method(env, listenerClazzRef->getClass(), "deviceConnected", _deviceStatusMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceConnected ) {
            throw InternalError("BluetoothDeviceDiscoveryListener has no deviceConnected"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceDisconnected = search_method(env, listenerClazzRef->getClass(), "deviceDisconnected", _deviceStatusMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceDisconnected ) {
            throw InternalError("BluetoothDeviceDiscoveryListener has no deviceDisconnected"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
    }

    void deviceFound(DBTAdapter const &a, std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** Native Adapter Device FOUND__: %s\n", device->toString().c_str());
                fprintf(stderr, "Status DBTAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif
            (void)a;

            // Device(final long nativeInstance, final Adapter adptr, final String address, final String name)
            const jstring addr = from_string_to_jstring(env, device->getAddressString());
            const jstring name = from_string_to_jstring(env, device->getName());
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            jobject jDevice = env->NewObject(deviceClazzRef->getClass(), deviceClazzCtor,
                    (jlong)device.get(), adapterObjRef, addr, name, (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            JNIGlobalRef::check(jDevice, E_FILE_LINE);
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);

            env->CallVoidMethod(listenerObjRef->getObject(), mDeviceFound,
                    JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef), (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
        } catch(...) {
            rethrow_and_raise_java_exception(env);
        }
    }
    void deviceUpdated(DBTAdapter const &a, std::shared_ptr<DBTDevice> device, const uint64_t timestamp, const EIRDataType updateMask) override {
        JNIEnv *env = *jni_env;
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** Native Adapter Device UPDATED: %s of %s\n", direct_bt::eirDataMaskToString(updateMask).c_str(), device->toString().c_str());
                fprintf(stderr, "Status DBTAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif
            (void)a;
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);
            env->SetLongField(JavaGlobalObj::GetObject(jDeviceRef), deviceClazzTSUpdateField, (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            jobject eirDataType = env->CallStaticObjectMethod(eirDataTypeClazzRef->getClass(), eirDataTypeClazzCreate, (jint)updateMask);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            env->CallVoidMethod(listenerObjRef->getObject(), mDeviceUpdated,
                    JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef), (jlong)timestamp, eirDataType);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
        } catch(...) {
            rethrow_and_raise_java_exception(env);
        }
    }
    void deviceConnected(DBTAdapter const &a, std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** DBTAdapter Device CONNECTED: %s\n", device->toString().c_str());
                fprintf(stderr, "Status DBTAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif
            (void)a;
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);
            env->SetLongField(JavaGlobalObj::GetObject(jDeviceRef), deviceClazzTSUpdateField, (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            env->CallVoidMethod(listenerObjRef->getObject(), mDeviceConnected,
                    JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef), (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
        } catch(...) {
            rethrow_and_raise_java_exception(env);
        }
    }
    void deviceDisconnected(DBTAdapter const &a, std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** DBTAdapter Device DISCONNECTED: %s\n", device->toString().c_str());
                fprintf(stderr, "Status DBTAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif
            (void)a;
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);
            env->SetLongField(JavaGlobalObj::GetObject(jDeviceRef), deviceClazzTSUpdateField, (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            env->CallVoidMethod(listenerObjRef->getObject(), mDeviceDisconnected,
                    JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef), (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
        } catch(...) {
            rethrow_and_raise_java_exception(env);
        }
    }
};

void Java_direct_1bt_tinyb_DBTAdapter_initImpl(JNIEnv *env, jobject obj, jobject deviceDiscoveryListener)
{
    // org.tinyb.BluetoothDeviceDiscoveryListener
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        // set our callback discovery listener.
        DeviceStatusCallbackListener *l = new DeviceStatusCallbackListener(env, adapter, deviceDiscoveryListener);
        adapter->setDeviceStatusListener(std::shared_ptr<DBTDeviceStatusListener>(l));
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_openImpl(JNIEnv *env, jobject obj) {
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        DBG_PRINT("Java_direct_1bt_tinyb_DBTAdapter_deleteImpl %s", adapter->toString().c_str());
        std::shared_ptr<direct_bt::HCISession> session = adapter->open();
        if( nullptr == session ) {
            throw BluetoothException("Couldn't open adapter "+adapter->toString(), E_FILE_LINE);
        }
        return JNI_TRUE;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

void Java_direct_1bt_tinyb_DBTAdapter_deleteImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        DBG_PRINT("Java_direct_1bt_tinyb_DBTAdapter_deleteImpl %s", adapter->toString().c_str());
        adapter->setDeviceStatusListener(nullptr);
        delete adapter;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_startDiscoveryImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        return adapter->startDiscovery();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_stopDiscoveryImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        adapter->stopDiscovery();
        return JNI_TRUE;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jobject Java_direct_1bt_tinyb_DBTAdapter_getDiscoveredDevicesImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        std::vector<std::shared_ptr<DBTDevice>> array = adapter->getDiscoveredDevices();
        return convert_vector_to_jobject(env, array);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jint Java_direct_1bt_tinyb_DBTAdapter_removeDevicesImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        return adapter->removeDiscoveredDevices();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

jlong Java_direct_1bt_tinyb_DBTAdapter_addDiscoveringNotificationsImpl(JNIEnv *env, jobject obj, jobject javaCallback)
{
    // org.tinyb.BluetoothDeviceDiscoveryListener
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        DBTManager & mgmt = adapter->getManager();
        std::shared_ptr<JNIGlobalRef> javaCallback_ptr(new JNIGlobalRef(javaCallback));

#if 1
        // this function instance satisfies to be key for identity,
        // as it is uniquely created for this javaCallback.
        bool(*nativeCallback)(std::shared_ptr<JNIGlobalRef>, std::shared_ptr<MgmtEvent>) =
                [](std::shared_ptr<JNIGlobalRef> javaCallback_ptr, std::shared_ptr<MgmtEvent> e)->bool {
            const MgmtEvtDiscovering &event = *static_cast<const MgmtEvtDiscovering *>(e.get());

            jclass notification = search_class(*jni_env, **javaCallback_ptr);
            jmethodID  method = search_method(*jni_env, notification, "run", "(Ljava/lang/Object;)V", false);
            jni_env->DeleteLocalRef(notification);

            jclass boolean_cls = search_class(*jni_env, "java/lang/Boolean");
            jmethodID constructor = search_method(*jni_env, boolean_cls, "<init>", "(Z)V", false);

            jobject result = jni_env->NewObject(boolean_cls, constructor, event.getEnabled() ? JNI_TRUE : JNI_FALSE);
            jni_env->DeleteLocalRef(boolean_cls);

            jni_env->CallVoidMethod(**javaCallback_ptr, method, result);
            jni_env->DeleteLocalRef(result);
            return true;
        };
        mgmt.addMgmtEventCallback(adapter->dev_id, MgmtEvent::Opcode::DISCOVERING, bindCaptureFunc(javaCallback_ptr, nativeCallback, false));
        // hack to convert function pointer to void *: '*((void**)&function)'
        return (jlong)( *((void**)&nativeCallback) );
#elif 0
        const uint64_t id = (jlong)obj;
        std::function<bool(std::shared_ptr<MgmtEvent> e)> nativeCallback = [javaCallback_ptr](std::shared_ptr<MgmtEvent> e)->bool {
            const MgmtEvtDiscovering &event = *static_cast<const MgmtEvtDiscovering *>(e.get());

            jclass notification = search_class(*jni_env, **javaCallback_ptr);
            jmethodID  method = search_method(*jni_env, notification, "run", "(Ljava/lang/Object;)V", false);
            jni_env->DeleteLocalRef(notification);

            jclass boolean_cls = search_class(*jni_env, "java/lang/Boolean");
            jmethodID constructor = search_method(*jni_env, boolean_cls, "<init>", "(Z)V", false);

            jobject result = jni_env->NewObject(boolean_cls, constructor, event.getEnabled() ? JNI_TRUE : JNI_FALSE);
            jni_env->DeleteLocalRef(boolean_cls);

            jni_env->CallVoidMethod(**javaCallback_ptr, method, result);
            jni_env->DeleteLocalRef(result);
            return true;
        };
        mgmt.addMgmtEventCallback(adapter->dev_id, MgmtEvent::Opcode::DISCOVERING, bindStdFunc(id, nativeCallback));
        return id;
#else
        // Capturing Lambda -> EventCallbackFunc type issues, need to learn how std::function does it!
        typedef bool(*EventCallbackFunc)(std::shared_ptr<MgmtEvent>);
        EventCallbackFunc nativeCallback = [javaCallback_ptr](std::shared_ptr<MgmtEvent> e)->bool {
            const MgmtEvtDiscovering &event = *static_cast<const MgmtEvtDiscovering *>(e.get());

            jclass notification = search_class(*jni_env, **javaCallback_ptr);
            jmethodID  method = search_method(*jni_env, notification, "run", "(Ljava/lang/Object;)V", false);
            jni_env->DeleteLocalRef(notification);

            jclass boolean_cls = search_class(*jni_env, "java/lang/Boolean");
            jmethodID constructor = search_method(*jni_env, boolean_cls, "<init>", "(Z)V", false);

            jobject result = jni_env->NewObject(boolean_cls, constructor, event.getEnabled() ? JNI_TRUE : JNI_FALSE);
            jni_env->DeleteLocalRef(boolean_cls);

            jni_env->CallVoidMethod(**javaCallback_ptr, method, result);
            jni_env->DeleteLocalRef(result);
            return true;
        };
        mgmt.addMgmtEventCallback(adapter->dev_id, MgmtEvent::Opcode::DISCOVERING, bindPlainFunc(nativeCallback));
        // hack to convert function pointer to void *: '*((void**)&function)'
        return (jlong)( *((void**)&nativeCallback) );
#endif
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

int Java_direct_1bt_tinyb_DBTAdapter_removeDiscoveringNotificationsImpl(JNIEnv *env, jobject obj, jlong jNativeCallback)
{
    // org.tinyb.BluetoothDeviceDiscoveryListener
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        DBTManager & mgmt = adapter->getManager();
#if 1
        bool(*nativeCallback)(std::shared_ptr<JNIGlobalRef>, std::shared_ptr<MgmtEvent>) =
                (bool(*)(std::shared_ptr<JNIGlobalRef>, std::shared_ptr<MgmtEvent>)) ( void *) jNativeCallback;
        return mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, bindCaptureFunc(std::shared_ptr<JNIGlobalRef>(nullptr), nativeCallback, false));
#elif 0
        const uint64_t id = (uint64_t)jNativeCallback;
        return mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, bindStdFunc<bool, std::shared_ptr<MgmtEvent>>(id));
#else
        bool(*nativeCallback)(std::shared_ptr<MgmtEvent>) = (bool(*)(std::shared_ptr<MgmtEvent>)) ( (void *)jNativeCallback );
        return mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, bindPlainFunc(nativeCallback));
#endif
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

jlong Java_direct_1bt_tinyb_DBTAdapter_addPoweredNotificationsImpl(JNIEnv *env, jobject obj, jobject javaCallback)
{
    // org.tinyb.BluetoothDeviceDiscoveryListener
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        DBTManager & mgmt = adapter->getManager();
        std::shared_ptr<JNIGlobalRef> javaCallback_ptr(new JNIGlobalRef(javaCallback));

        // this function instance satisfies to be key for identity,
        // as it is uniquely created for this javaCallback.
        bool(*nativeCallback)(std::shared_ptr<JNIGlobalRef>, std::shared_ptr<MgmtEvent>) =
                [](std::shared_ptr<JNIGlobalRef> javaCallback_ptr, std::shared_ptr<MgmtEvent> e)->bool {
            const MgmtEvtNewSettings &event = *static_cast<const MgmtEvtNewSettings *>(e.get());

            jclass notification = search_class(*jni_env, **javaCallback_ptr);
            jmethodID  method = search_method(*jni_env, notification, "run", "(Ljava/lang/Object;)V", false);
            jni_env->DeleteLocalRef(notification);

            jclass boolean_cls = search_class(*jni_env, "java/lang/Boolean");
            jmethodID constructor = search_method(*jni_env, boolean_cls, "<init>", "(Z)V", false);

            jobject result = jni_env->NewObject(boolean_cls, constructor, ( 0 != ( event.getSettings() & MGMT_SETTING_POWERED ) ) ? JNI_TRUE : JNI_FALSE);
            jni_env->DeleteLocalRef(boolean_cls);

            jni_env->CallVoidMethod(**javaCallback_ptr, method, result);
            jni_env->DeleteLocalRef(result);
            return true;
        };
        mgmt.addMgmtEventCallback(adapter->dev_id, MgmtEvent::Opcode::NEW_SETTINGS, bindCaptureFunc(javaCallback_ptr, nativeCallback, false));
        // hack to convert function pointer to void *: '*((void**)&function)'
        return (jlong)( *((void**)&nativeCallback) );
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

int Java_direct_1bt_tinyb_DBTAdapter_removePoweredNotificationsImpl(JNIEnv *env, jobject obj, jlong jNativeCallback)
{
    // org.tinyb.BluetoothDeviceDiscoveryListener
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        DBTManager & mgmt = adapter->getManager();

        bool(*nativeCallback)(std::shared_ptr<JNIGlobalRef>, std::shared_ptr<MgmtEvent>) =
                (bool(*)(std::shared_ptr<JNIGlobalRef>, std::shared_ptr<MgmtEvent>)) ( void *) jNativeCallback;
        return mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::NEW_SETTINGS, bindCaptureFunc(std::shared_ptr<JNIGlobalRef>(nullptr), nativeCallback, false));
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

