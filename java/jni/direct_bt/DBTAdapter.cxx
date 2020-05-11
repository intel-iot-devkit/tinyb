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

#include "direct_bt/DBTAdapter.hpp"

using namespace direct_bt;

static const std::string _adapterSettingsClassName("org/tinyb/AdapterSettings");
static const std::string _adapterSettingsClazzCtorArgs("(I)V");
static const std::string _eirDataTypeSetClassName("org/tinyb/EIRDataTypeSet");
static const std::string _eirDataTypeSetClazzCtorArgs("(I)V");
static const std::string _deviceClazzCtorArgs("(JLdirect_bt/tinyb/DBTAdapter;Ljava/lang/String;Ljava/lang/String;J)V");
static const std::string _adapterSettingsChangedMethodArgs("(Lorg/tinyb/BluetoothAdapter;Lorg/tinyb/AdapterSettings;Lorg/tinyb/AdapterSettings;Lorg/tinyb/AdapterSettings;J)V");
static const std::string _deviceStatusMethodArgs("(Lorg/tinyb/BluetoothAdapter;Lorg/tinyb/BluetoothDevice;J)V");
static const std::string _deviceStatusUpdateMethodArgs("(Lorg/tinyb/BluetoothAdapter;Lorg/tinyb/BluetoothDevice;JLorg/tinyb/EIRDataTypeSet;)V");

class AdapterStatusCallbackListener : public DBTAdapterStatusListener {
  private:
    /**
        package org.tinyb;

        public interface AdapterStatusListener {
            public void adapterSettingsChanged(final BluetoothAdapter adapter,
                                               final AdapterSetting oldmask, final AdapterSetting newmask,
                                               final AdapterSetting changedmask, final long timestamp);
            public void deviceFound(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
            public void deviceUpdated(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp, final EIRDataType updateMask);
            public void deviceConnected(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
            public void deviceDisconnected(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
        };
    */
    std::shared_ptr<JavaAnonObj> adapterObjRef;
    std::unique_ptr<JNIGlobalRef> adapterSettingsClazzRef;
    jmethodID adapterSettingsClazzCtor;
    std::unique_ptr<JNIGlobalRef> eirDataTypeSetClazzRef;
    jmethodID eirDataTypeSetClazzCtor;
    std::unique_ptr<JNIGlobalRef> deviceClazzRef;
    jmethodID deviceClazzCtor;
    jfieldID deviceClazzTSUpdateField;
    std::unique_ptr<JNIGlobalRef> listenerObjRef;
    std::unique_ptr<JNIGlobalRef> listenerClazzRef;
    jmethodID  mAdapterSettingsChanged = nullptr;
    jmethodID  mDeviceFound = nullptr;
    jmethodID  mDeviceUpdated = nullptr;
    jmethodID  mDeviceConnected = nullptr;
    jmethodID  mDeviceDisconnected = nullptr;

  public:

    AdapterStatusCallbackListener(JNIEnv *env, DBTAdapter *adapter, jobject statusListener) {
        adapterObjRef = adapter->getJavaObject();
        JavaGlobalObj::check(adapterObjRef, E_FILE_LINE);

        // adapterSettingsClazzRef, adapterSettingsClazzCtor
        {
            jclass adapterSettingsClazz = search_class(env, _adapterSettingsClassName.c_str());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == adapterSettingsClazz ) {
                throw InternalError("DBTDevice::java_class not found: "+_adapterSettingsClassName, E_FILE_LINE);
            }
            adapterSettingsClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(adapterSettingsClazz));
            env->DeleteLocalRef(adapterSettingsClazz);
        }
        adapterSettingsClazzCtor = search_method(env, adapterSettingsClazzRef->getClass(), "<init>", _adapterSettingsClazzCtorArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == adapterSettingsClazzCtor ) {
            throw InternalError("AdapterSettings ctor not found: "+_adapterSettingsClassName+".<init>"+_adapterSettingsClazzCtorArgs, E_FILE_LINE);
        }

        // eirDataTypeSetClazzRef, eirDataTypeSetClazzCtor
        {
            jclass eirDataTypeSetClazz = search_class(env, _eirDataTypeSetClassName.c_str());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == eirDataTypeSetClazz ) {
                throw InternalError("DBTDevice::java_class not found: "+_eirDataTypeSetClassName, E_FILE_LINE);
            }
            eirDataTypeSetClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(eirDataTypeSetClazz));
            env->DeleteLocalRef(eirDataTypeSetClazz);
        }
        eirDataTypeSetClazzCtor = search_method(env, eirDataTypeSetClazzRef->getClass(), "<init>", _eirDataTypeSetClazzCtorArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == eirDataTypeSetClazzCtor ) {
            throw InternalError("EIRDataType ctor not found: "+_eirDataTypeSetClassName+".<init>"+_eirDataTypeSetClazzCtorArgs, E_FILE_LINE);
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

        listenerObjRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(statusListener));
        {
            jclass listenerClazz = search_class(env, listenerObjRef->getObject());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == listenerClazz ) {
                throw InternalError("AdapterStatusListener not found", E_FILE_LINE);
            }
            listenerClazzRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(listenerClazz));
            env->DeleteLocalRef(listenerClazz);
        }


        mAdapterSettingsChanged = search_method(env, listenerClazzRef->getClass(), "adapterSettingsChanged", _adapterSettingsChangedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mAdapterSettingsChanged ) {
            throw InternalError("AdapterStatusListener has no adapterSettingsChanged"+_adapterSettingsChangedMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceFound = search_method(env, listenerClazzRef->getClass(), "deviceFound", _deviceStatusMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceFound ) {
            throw InternalError("AdapterStatusListener has no deviceFound"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceUpdated = search_method(env, listenerClazzRef->getClass(), "deviceUpdated", _deviceStatusUpdateMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceUpdated ) {
            throw InternalError("AdapterStatusListener has no deviceUpdated"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceConnected = search_method(env, listenerClazzRef->getClass(), "deviceConnected", _deviceStatusMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceConnected ) {
            throw InternalError("AdapterStatusListener has no deviceConnected"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceDisconnected = search_method(env, listenerClazzRef->getClass(), "deviceDisconnected", _deviceStatusMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceDisconnected ) {
            throw InternalError("AdapterStatusListener has no deviceDisconnected"+_deviceStatusMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
    }

    void adapterSettingsChanged(DBTAdapter const &a, const AdapterSetting oldmask, const AdapterSetting newmask,
                                const AdapterSetting changedmask, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        try {
            #ifdef VERBOSE_ON
                fprintf(stderr, "****** Native Adapter SETTINGS_CHANGED: %s -> %s, changed %s\n",
                        direct_bt::adapterSettingsToString(oldmask).c_str(),
                        direct_bt::adapterSettingsToString(newmask).c_str(),
                        direct_bt::adapterSettingsToString(changedmask).c_str());
                fprintf(stderr, "Status DBTAdapter:\n");
                fprintf(stderr, "%s\n", a.toString().c_str());
            #endif
            (void)a;
            jobject adapterSettingOld = env->NewObject(adapterSettingsClazzRef->getClass(), adapterSettingsClazzCtor,  (jint)oldmask);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            JNIGlobalRef::check(adapterSettingOld, E_FILE_LINE);

            jobject adapterSettingNew = env->NewObject(adapterSettingsClazzRef->getClass(), adapterSettingsClazzCtor,  (jint)newmask);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            JNIGlobalRef::check(adapterSettingNew, E_FILE_LINE);

            jobject adapterSettingChanged = env->NewObject(adapterSettingsClazzRef->getClass(), adapterSettingsClazzCtor,  (jint)changedmask);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            JNIGlobalRef::check(adapterSettingChanged, E_FILE_LINE);

            env->CallVoidMethod(listenerObjRef->getObject(), mAdapterSettingsChanged,
                    JavaGlobalObj::GetObject(adapterObjRef), adapterSettingOld, adapterSettingNew, adapterSettingChanged, (jlong)timestamp);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
        } catch(...) {
            rethrow_and_raise_java_exception(env);
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

            jobject eirDataTypeSet = env->NewObject(eirDataTypeSetClazzRef->getClass(), eirDataTypeSetClazzCtor, (jint)updateMask);
            if( java_exception_check(env, E_FILE_LINE) ) { return; }
            JNIGlobalRef::check(eirDataTypeSet, E_FILE_LINE);

            env->CallVoidMethod(listenerObjRef->getObject(), mDeviceUpdated,
                    JavaGlobalObj::GetObject(adapterObjRef), JavaGlobalObj::GetObject(jDeviceRef), (jlong)timestamp, eirDataTypeSet);
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

jboolean Java_direct_1bt_tinyb_DBTAdapter_addStatusListener(JNIEnv *env, jobject obj, jobject statusListener)
{
    try {
        if( nullptr == statusListener ) {
            throw IllegalArgumentException("statusListener is null", E_FILE_LINE);
        }
        {
            AdapterStatusCallbackListener * pre =
                    getObjectRef<AdapterStatusCallbackListener>(env, statusListener, "nativeInstance");
            if( nullptr != pre ) {
                WARN_PRINT("statusListener's nativeInstance not null, already in use");
                return false;
            }
        }
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        std::shared_ptr<DBTAdapterStatusListener> l =
                std::shared_ptr<DBTAdapterStatusListener>( new AdapterStatusCallbackListener(env, adapter, statusListener) );

        if( adapter->addStatusListener( l ) ) {
            setInstance(env, statusListener, l.get());
            return JNI_TRUE;
        }
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_removeStatusListener(JNIEnv *env, jobject obj, jobject statusListener)
{
    try {
        if( nullptr == statusListener ) {
            throw IllegalArgumentException("statusListener is null", E_FILE_LINE);
        }
        const AdapterStatusCallbackListener * pre =
                getObjectRef<AdapterStatusCallbackListener>(env, statusListener, "nativeInstance");
        if( nullptr == pre ) {
            WARN_PRINT("statusListener's nativeInstance is null, not in use");
            return false;
        }
        setObjectRef<AdapterStatusCallbackListener>(env, statusListener, nullptr, "nativeInstance");

        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        // set our callback discovery listener.
        if( ! adapter->removeStatusListener( pre ) ) {
            throw InternalError("Failed to remove statusListener with nativeInstance", E_FILE_LINE);
        }
        return true;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
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

//
// Discovering
//

static void disableDiscoveringNotifications(JNIEnv *env, jobject obj, DBTManager &mgmt)
{
    InvocationFunc<bool, std::shared_ptr<MgmtEvent>> * funcptr =
            getObjectRef<InvocationFunc<bool, std::shared_ptr<MgmtEvent>>>(env, obj, "discoveringNotificationRef");
    if( nullptr != funcptr ) {
        FunctionDef<bool, std::shared_ptr<MgmtEvent>> funcDef( funcptr );
        funcptr = nullptr;
        setObjectRef(env, obj, funcptr, "discoveringNotificationRef"); // clear java ref
        int count;
        if( 1 != ( count = mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, funcDef) ) ) {
            throw direct_bt::InternalError(std::string("removeMgmtEventCallback of ")+funcDef.toString()+" not 1 but "+std::to_string(count), E_FILE_LINE);
        }
    }
}
void Java_direct_1bt_tinyb_DBTAdapter_disableDiscoveringNotifications(JNIEnv *env, jobject obj)
{
    // org.tinyb.AdapterStatusListener
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        DBTManager & mgmt = adapter->getManager();

        disableDiscoveringNotifications(env, obj, mgmt);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}
void Java_direct_1bt_tinyb_DBTAdapter_enableDiscoveringNotifications(JNIEnv *env, jobject obj, jobject javaCallback)
{
    // org.tinyb.AdapterStatusListener
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        DBTManager & mgmt = adapter->getManager();

        disableDiscoveringNotifications(env, obj, mgmt);

        bool(*nativeCallback)(JNIGlobalRef&, std::shared_ptr<MgmtEvent>) =
                [](JNIGlobalRef& javaCallback_ref, std::shared_ptr<MgmtEvent> e)->bool {
            const MgmtEvtDiscovering &event = *static_cast<const MgmtEvtDiscovering *>(e.get());

            jclass notification = search_class(*jni_env, *javaCallback_ref);
            jmethodID  method = search_method(*jni_env, notification, "run", "(Ljava/lang/Object;)V", false);
            jni_env->DeleteLocalRef(notification);

            jclass boolean_cls = search_class(*jni_env, "java/lang/Boolean");
            jmethodID constructor = search_method(*jni_env, boolean_cls, "<init>", "(Z)V", false);

            jobject result = jni_env->NewObject(boolean_cls, constructor, event.getEnabled() ? JNI_TRUE : JNI_FALSE);
            jni_env->DeleteLocalRef(boolean_cls);

            jni_env->CallVoidMethod(*javaCallback_ref, method, result);
            jni_env->DeleteLocalRef(result);
            return true;
        };
        // move JNIGlobalRef into CaptureInvocationFunc and operator== includes javaCallback comparison
        FunctionDef<bool, std::shared_ptr<MgmtEvent>> funcDef = bindCaptureFunc(JNIGlobalRef(javaCallback), nativeCallback);
        setObjectRef(env, obj, funcDef.cloneFunction(), "discoveringNotificationRef"); // set java ref
        mgmt.addMgmtEventCallback(adapter->dev_id, MgmtEvent::Opcode::DISCOVERING, funcDef);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

//
// misc
//

void Java_direct_1bt_tinyb_DBTAdapter_setPowered(JNIEnv *env, jobject obj, jboolean value) {
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        adapter->setPowered(JNI_TRUE == value ? true : false);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jstring Java_direct_1bt_tinyb_DBTAdapter_getAlias(JNIEnv *env, jobject obj) {
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        return from_string_to_jstring(env, adapter->getLocalName().getName());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

void Java_direct_1bt_tinyb_DBTAdapter_setAlias(JNIEnv *env, jobject obj, jstring jnewalias) {
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        std::string newalias = from_jstring_to_string(env, jnewalias);
        adapter->setLocalName(newalias, std::string());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

void Java_direct_1bt_tinyb_DBTAdapter_setDiscoverable(JNIEnv *env, jobject obj, jboolean value) {
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        adapter->setDiscoverable(JNI_TRUE == value ? true : false);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jobject Java_direct_1bt_tinyb_DBTAdapter_connectDevice(JNIEnv *env, jobject obj, jstring jaddress, jstring jaddressType) {
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        std::string saddress = from_jstring_to_string(env, jaddress);
        EUI48 address(saddress);
        std::shared_ptr<DBTDevice> device = adapter->findDiscoveredDevice(address);
        if( nullptr != device ) {
            const BDAddressType addressType = fromJavaAdressTypeToBDAddressType(env, jaddressType);
            const BDAddressType addressTypeDevice = device->getAddressType();
            if( addressTypeDevice != addressType ) {
                // oops?
                WARN_PRINT("DBTAdapter::connectDevice: AddressType mismatch, ignoring request: Requested %s, Device %s %s",
                        getBDAddressTypeString(addressType).c_str(), getBDAddressTypeString(addressTypeDevice).c_str(),
                        device->toString().c_str());
            }
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);

            device->defaultConnect();
            return JavaGlobalObj::GetObject(jDeviceRef);
        }
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

void Java_direct_1bt_tinyb_DBTAdapter_setPairable(JNIEnv *env, jobject obj, jboolean value) {
    try {
        DBTAdapter *adapter = getInstance<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        adapter->setBondable(JNI_TRUE == value ? true : false);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}
