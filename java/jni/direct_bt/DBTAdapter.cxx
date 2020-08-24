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

// #define VERBOSE_ON 1
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
static const std::string _hciStatusCodeClassName("org/tinyb/HCIStatusCode");
static const std::string _hciStatusCodeClazzGetArgs("(B)Lorg/tinyb/HCIStatusCode;");
static const std::string _deviceClazzCtorArgs("(JLdirect_bt/tinyb/DBTAdapter;Ljava/lang/String;IILjava/lang/String;J)V");

static const std::string _adapterSettingsChangedMethodArgs("(Lorg/tinyb/BluetoothAdapter;Lorg/tinyb/AdapterSettings;Lorg/tinyb/AdapterSettings;Lorg/tinyb/AdapterSettings;J)V");
static const std::string _discoveringChangedMethodArgs("(Lorg/tinyb/BluetoothAdapter;ZZJ)V");
static const std::string _deviceFoundMethodArgs("(Lorg/tinyb/BluetoothDevice;J)V");
static const std::string _deviceUpdatedMethodArgs("(Lorg/tinyb/BluetoothDevice;Lorg/tinyb/EIRDataTypeSet;J)V");
static const std::string _deviceConnectedMethodArgs("(Lorg/tinyb/BluetoothDevice;SJ)V");
static const std::string _deviceDisconnectedMethodArgs("(Lorg/tinyb/BluetoothDevice;Lorg/tinyb/HCIStatusCode;SJ)V");

class JNIAdapterStatusListener : public AdapterStatusListener {
  private:
    /**
        package org.tinyb;

        public abstract class AdapterStatusListener {
            private long nativeInstance;

            public void adapterSettingsChanged(final BluetoothAdapter adapter,
                                               final AdapterSettings oldmask, final AdapterSettings newmask,
                                               final AdapterSettings changedmask, final long timestamp) { }
            public void discoveringChanged(final BluetoothAdapter adapter, final boolean enabled, final boolean keepAlive, final long timestamp) { }
            public void deviceFound(final BluetoothDevice device, final long timestamp) { }
            public void deviceUpdated(final BluetoothDevice device, final EIRDataTypeSet updateMask, final long timestamp) { }
            public void deviceConnected(final BluetoothDevice device, final short handle, final long timestamp) { }
            public void deviceDisconnected(final BluetoothDevice device, final HCIStatusCode reason, final short handle, final long timestamp) { }

        };
    */
    static std::atomic<int> iname_next;
    int const iname;
    DBTDevice const * const deviceMatchRef;
    std::shared_ptr<JavaAnonObj> adapterObjRef;
    JNIGlobalRef adapterSettingsClazzRef;
    jmethodID adapterSettingsClazzCtor;
    JNIGlobalRef eirDataTypeSetClazzRef;
    jmethodID eirDataTypeSetClazzCtor;
    JNIGlobalRef hciErrorCodeClazzRef;
    jmethodID hciErrorCodeClazzGet;
    JNIGlobalRef deviceClazzRef;
    jmethodID deviceClazzCtor;
    jfieldID deviceClazzTSLastDiscoveryField;
    jfieldID deviceClazzTSLastUpdateField;
    jfieldID deviceClazzConnectionHandleField;
    JNIGlobalRef listenerObjRef;
    jmethodID  mAdapterSettingsChanged = nullptr;
    jmethodID  mDiscoveringChanged = nullptr;
    jmethodID  mDeviceFound = nullptr;
    jmethodID  mDeviceUpdated = nullptr;
    jmethodID  mDeviceConnected= nullptr;
    jmethodID  mDeviceDisconnected = nullptr;

  public:

    std::string toString() const override {
        const std::string devMatchAddr = nullptr != deviceMatchRef ? deviceMatchRef->address.toString() : "nil";
        return "JNIAdapterStatusListener[this "+aptrHexString(this)+", iname "+std::to_string(iname)+", devMatchAddr "+devMatchAddr+"]";
    }

    JNIAdapterStatusListener(JNIEnv *env, DBTAdapter *adapter, jobject statusListener, const DBTDevice * _deviceMatchRef)
    : iname(iname_next.fetch_add(1)), deviceMatchRef(_deviceMatchRef), listenerObjRef(statusListener)
    {
        adapterObjRef = adapter->getJavaObject();
        JavaGlobalObj::check(adapterObjRef, E_FILE_LINE);

        jclass listenerClazz = search_class(env, listenerObjRef.getObject());
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == listenerClazz ) {
            throw InternalError("AdapterStatusListener not found", E_FILE_LINE);
        }

        // adapterSettingsClazzRef, adapterSettingsClazzCtor
        {
            jclass adapterSettingsClazz = search_class(env, _adapterSettingsClassName.c_str());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == adapterSettingsClazz ) {
                throw InternalError("DBTDevice::java_class not found: "+_adapterSettingsClassName, E_FILE_LINE);
            }
            adapterSettingsClazzRef = JNIGlobalRef(adapterSettingsClazz);
            env->DeleteLocalRef(adapterSettingsClazz);
        }
        adapterSettingsClazzCtor = search_method(env, adapterSettingsClazzRef.getClass(), "<init>", _adapterSettingsClazzCtorArgs.c_str(), false);
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
            eirDataTypeSetClazzRef = JNIGlobalRef(eirDataTypeSetClazz);
            env->DeleteLocalRef(eirDataTypeSetClazz);
        }
        eirDataTypeSetClazzCtor = search_method(env, eirDataTypeSetClazzRef.getClass(), "<init>", _eirDataTypeSetClazzCtorArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == eirDataTypeSetClazzCtor ) {
            throw InternalError("EIRDataType ctor not found: "+_eirDataTypeSetClassName+".<init>"+_eirDataTypeSetClazzCtorArgs, E_FILE_LINE);
        }

        // hciErrorCodeClazzRef, hciErrorCodeClazzGet
        {
            jclass hciErrorCodeClazz = search_class(env, _hciStatusCodeClassName.c_str());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == hciErrorCodeClazz ) {
                throw InternalError("DBTDevice::java_class not found: "+_hciStatusCodeClassName, E_FILE_LINE);
            }
            hciErrorCodeClazzRef = JNIGlobalRef(hciErrorCodeClazz);
            env->DeleteLocalRef(hciErrorCodeClazz);
        }
        hciErrorCodeClazzGet = search_method(env, hciErrorCodeClazzRef.getClass(), "get", _hciStatusCodeClazzGetArgs.c_str(), true);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == hciErrorCodeClazzGet ) {
            throw InternalError("EIRDataType ctor not found: "+_hciStatusCodeClassName+".get"+_hciStatusCodeClazzGetArgs, E_FILE_LINE);
        }

        // deviceClazzRef, deviceClazzCtor
        {
            jclass deviceClazz = search_class(env, DBTDevice::java_class().c_str());
            java_exception_check_and_throw(env, E_FILE_LINE);
            if( nullptr == deviceClazz ) {
                throw InternalError("DBTDevice::java_class not found: "+DBTDevice::java_class(), E_FILE_LINE);
            }
            deviceClazzRef = JNIGlobalRef(deviceClazz);
            env->DeleteLocalRef(deviceClazz);
        }
        deviceClazzCtor = search_method(env, deviceClazzRef.getClass(), "<init>", _deviceClazzCtorArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == deviceClazzCtor ) {
            throw InternalError("DBTDevice::java_class ctor not found: "+DBTDevice::java_class()+".<init>"+_deviceClazzCtorArgs, E_FILE_LINE);
        }
        deviceClazzTSLastDiscoveryField = env->GetFieldID(deviceClazzRef.getClass(), "ts_last_discovery", "J");
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == deviceClazzTSLastDiscoveryField ) {
            throw InternalError("DBTDevice::java_class field not found: "+DBTDevice::java_class()+".ts_last_discovery", E_FILE_LINE);
        }
        deviceClazzTSLastUpdateField = env->GetFieldID(deviceClazzRef.getClass(), "ts_last_update", "J");
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == deviceClazzTSLastUpdateField ) {
            throw InternalError("DBTDevice::java_class field not found: "+DBTDevice::java_class()+".ts_last_update", E_FILE_LINE);
        }
        deviceClazzConnectionHandleField = env->GetFieldID(deviceClazzRef.getClass(), "hciConnHandle", "S");
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == deviceClazzConnectionHandleField ) {
            throw InternalError("DBTDevice::java_class field not found: "+DBTDevice::java_class()+".hciConnHandle", E_FILE_LINE);
        }

        mAdapterSettingsChanged = search_method(env, listenerClazz, "adapterSettingsChanged", _adapterSettingsChangedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mAdapterSettingsChanged ) {
            throw InternalError("AdapterStatusListener has no adapterSettingsChanged"+_adapterSettingsChangedMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDiscoveringChanged = search_method(env, listenerClazz, "discoveringChanged", _discoveringChangedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDiscoveringChanged ) {
            throw InternalError("AdapterStatusListener has no discoveringChanged"+_discoveringChangedMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceFound = search_method(env, listenerClazz, "deviceFound", _deviceFoundMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceFound ) {
            throw InternalError("AdapterStatusListener has no deviceFound"+_deviceFoundMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceUpdated = search_method(env, listenerClazz, "deviceUpdated", _deviceUpdatedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceUpdated ) {
            throw InternalError("AdapterStatusListener has no deviceUpdated"+_deviceUpdatedMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceConnected = search_method(env, listenerClazz, "deviceConnected", _deviceConnectedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceConnected ) {
            throw InternalError("AdapterStatusListener has no deviceConnected"+_deviceConnectedMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
        mDeviceDisconnected = search_method(env, listenerClazz, "deviceDisconnected", _deviceDisconnectedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mDeviceDisconnected ) {
            throw InternalError("AdapterStatusListener has no deviceDisconnected"+_deviceDisconnectedMethodArgs+" method, for "+adapter->toString(), E_FILE_LINE);
        }
    }

    bool matchDevice(const DBTDevice & device) override {
        if( nullptr == deviceMatchRef ) {
            return true;
        }
        return device == *deviceMatchRef;
    }

    void adapterSettingsChanged(DBTAdapter const &a, const AdapterSetting oldmask, const AdapterSetting newmask,
                                const AdapterSetting changedmask, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        (void)a;
        jobject adapterSettingOld = env->NewObject(adapterSettingsClazzRef.getClass(), adapterSettingsClazzCtor,  (jint)oldmask);
        java_exception_check_and_throw(env, E_FILE_LINE);
        JNIGlobalRef::check(adapterSettingOld, E_FILE_LINE);

        jobject adapterSettingNew = env->NewObject(adapterSettingsClazzRef.getClass(), adapterSettingsClazzCtor,  (jint)newmask);
        java_exception_check_and_throw(env, E_FILE_LINE);
        JNIGlobalRef::check(adapterSettingNew, E_FILE_LINE);

        jobject adapterSettingChanged = env->NewObject(adapterSettingsClazzRef.getClass(), adapterSettingsClazzCtor,  (jint)changedmask);
        java_exception_check_and_throw(env, E_FILE_LINE);
        JNIGlobalRef::check(adapterSettingChanged, E_FILE_LINE);

        env->CallVoidMethod(listenerObjRef.getObject(), mAdapterSettingsChanged,
                JavaGlobalObj::GetObject(adapterObjRef), adapterSettingOld, adapterSettingNew, adapterSettingChanged, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);
    }

    void discoveringChanged(DBTAdapter const &a, const bool enabled, const bool keepAlive, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        (void)a;
        env->CallVoidMethod(listenerObjRef.getObject(), mDiscoveringChanged, JavaGlobalObj::GetObject(adapterObjRef),
                            (jboolean)enabled, (jboolean)keepAlive, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);
    }

    void deviceFound(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        jobject jdevice;
        std::shared_ptr<JavaAnonObj> jDeviceRef0 = device->getJavaObject();
        if( JavaGlobalObj::isValid(jDeviceRef0) ) {
            // Reuse Java instance
            jdevice = JavaGlobalObj::GetObject(jDeviceRef0);
        } else {
            // New Java instance
            // Device(final long nativeInstance, final Adapter adptr, final String address, final int intAddressType, final String name)
            const jstring addr = from_string_to_jstring(env, device->getAddressString());
            const jstring name = from_string_to_jstring(env, device->getName());
            java_exception_check_and_throw(env, E_FILE_LINE);
            jobject tmp_jdevice = env->NewObject(deviceClazzRef.getClass(), deviceClazzCtor,
                    (jlong)device.get(), JavaGlobalObj::GetObject(adapterObjRef), addr,
                    device->getAddressType(), device->getBLERandomAddressType(),
                    name, (jlong)timestamp);
            java_exception_check_and_throw(env, E_FILE_LINE);
            JNIGlobalRef::check(tmp_jdevice, E_FILE_LINE);
            std::shared_ptr<JavaAnonObj> jDeviceRef1 = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef1, E_FILE_LINE);
            jdevice = JavaGlobalObj::GetObject(jDeviceRef1);
            env->DeleteLocalRef(tmp_jdevice);
        }
        env->SetLongField(jdevice, deviceClazzTSLastDiscoveryField, (jlong)device->getLastDiscoveryTimestamp());
        java_exception_check_and_throw(env, E_FILE_LINE);
        env->CallVoidMethod(listenerObjRef.getObject(), mDeviceFound, jdevice, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);
    }

    void deviceUpdated(std::shared_ptr<DBTDevice> device, const EIRDataType updateMask, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
        JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);
        env->SetLongField(JavaGlobalObj::GetObject(jDeviceRef), deviceClazzTSLastUpdateField, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);

        jobject eirDataTypeSet = env->NewObject(eirDataTypeSetClazzRef.getClass(), eirDataTypeSetClazzCtor, (jint)updateMask);
        java_exception_check_and_throw(env, E_FILE_LINE);
        JNIGlobalRef::check(eirDataTypeSet, E_FILE_LINE);

        env->CallVoidMethod(listenerObjRef.getObject(), mDeviceUpdated, JavaGlobalObj::GetObject(jDeviceRef), eirDataTypeSet, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);
    }

    void deviceConnected(std::shared_ptr<DBTDevice> device, const uint16_t handle, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;

        jobject jdevice;
        std::shared_ptr<JavaAnonObj> jDeviceRef0 = device->getJavaObject();
        if( JavaGlobalObj::isValid(jDeviceRef0) ) {
            // Reuse Java instance
            jdevice = JavaGlobalObj::GetObject(jDeviceRef0);
        } else {
            // New Java instance
            // Device(final long nativeInstance, final Adapter adptr, final String address, final int intAddressType, final String name)
            const jstring addr = from_string_to_jstring(env, device->getAddressString());
            const jstring name = from_string_to_jstring(env, device->getName());
            java_exception_check_and_throw(env, E_FILE_LINE);
            jobject tmp_jdevice = env->NewObject(deviceClazzRef.getClass(), deviceClazzCtor,
                    (jlong)device.get(), JavaGlobalObj::GetObject(adapterObjRef), addr,
                    device->getAddressType(), device->getBLERandomAddressType(),
                    name, (jlong)timestamp);
            java_exception_check_and_throw(env, E_FILE_LINE);
            JNIGlobalRef::check(tmp_jdevice, E_FILE_LINE);
            std::shared_ptr<JavaAnonObj> jDeviceRef1 = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef1, E_FILE_LINE);
            jdevice = JavaGlobalObj::GetObject(jDeviceRef1);
            env->DeleteLocalRef(tmp_jdevice);
        }
        env->SetShortField(jdevice, deviceClazzConnectionHandleField, (jshort)handle);
        java_exception_check_and_throw(env, E_FILE_LINE);
        env->SetLongField(jdevice, deviceClazzTSLastDiscoveryField, (jlong)device->getLastDiscoveryTimestamp());
        java_exception_check_and_throw(env, E_FILE_LINE);
        env->SetLongField(jdevice, deviceClazzTSLastUpdateField, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);

        env->CallVoidMethod(listenerObjRef.getObject(), mDeviceConnected, jdevice, (jshort)handle, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);
    }
    void deviceDisconnected(std::shared_ptr<DBTDevice> device, const HCIStatusCode reason, const uint16_t handle, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;

        std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
        JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);
        jobject jdevice = JavaGlobalObj::GetObject(jDeviceRef);
        env->SetLongField(jdevice, deviceClazzTSLastUpdateField, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);

        jobject hciErrorCode = env->CallStaticObjectMethod(hciErrorCodeClazzRef.getClass(), hciErrorCodeClazzGet, (jbyte)static_cast<uint8_t>(reason));
        java_exception_check_and_throw(env, E_FILE_LINE);
        JNIGlobalRef::check(hciErrorCode, E_FILE_LINE);

        env->SetShortField(jdevice, deviceClazzConnectionHandleField, (jshort)0); // zero out, disconnected
        java_exception_check_and_throw(env, E_FILE_LINE);
        env->SetLongField(jdevice, deviceClazzTSLastUpdateField, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);

        env->CallVoidMethod(listenerObjRef.getObject(), mDeviceDisconnected, jdevice, hciErrorCode, (jshort)handle, (jlong)timestamp);
        java_exception_check_and_throw(env, E_FILE_LINE);
    }
};
std::atomic<int> JNIAdapterStatusListener::iname_next(0);

jboolean Java_direct_1bt_tinyb_DBTAdapter_addStatusListener(JNIEnv *env, jobject obj, jobject statusListener, jobject jdeviceMatch)
{
    try {
        if( nullptr == statusListener ) {
            throw IllegalArgumentException("JNIAdapterStatusListener::addStatusListener: statusListener is null", E_FILE_LINE);
        }
        {
            JNIAdapterStatusListener * pre =
                    getObjectRef<JNIAdapterStatusListener>(env, statusListener, "nativeInstance");
            if( nullptr != pre ) {
                WARN_PRINT("JNIAdapterStatusListener::addStatusListener: statusListener's nativeInstance not null, already in use");
                return false;
            }
        }
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        DBTDevice * deviceMatchRef = nullptr;
        if( nullptr != jdeviceMatch ) {
            deviceMatchRef = getDBTObject<DBTDevice>(env, jdeviceMatch);
            JavaGlobalObj::check(deviceMatchRef->getJavaObject(), E_FILE_LINE);
        }

        std::shared_ptr<AdapterStatusListener> l =
                std::shared_ptr<AdapterStatusListener>( new JNIAdapterStatusListener(env, adapter, statusListener, deviceMatchRef) );

        if( adapter->addStatusListener( l ) ) {
            setInstance(env, statusListener, l.get());
            return JNI_TRUE;
        }
        ERR_PRINT("JNIAdapterStatusListener::addStatusListener: FAILED: %s", l->toString().c_str());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    ERR_PRINT("JNIAdapterStatusListener::addStatusListener: FAILED XX");
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_removeStatusListener(JNIEnv *env, jobject obj, jobject statusListener)
{
    try {
        if( nullptr == statusListener ) {
            throw IllegalArgumentException("statusListener is null", E_FILE_LINE);
        }
        const JNIAdapterStatusListener * pre =
                getObjectRef<JNIAdapterStatusListener>(env, statusListener, "nativeInstance");
        if( nullptr == pre ) {
            DBG_PRINT("statusListener's nativeInstance is null, not in use");
            return false;
        }
        setObjectRef<JNIAdapterStatusListener>(env, statusListener, nullptr, "nativeInstance");

        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        if( ! adapter->removeStatusListener( pre ) ) {
            WARN_PRINT("Failed to remove statusListener with nativeInstance: %p at %s", pre, adapter->toString().c_str());
            return false;
        }
        return true;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jint Java_direct_1bt_tinyb_DBTAdapter_removeAllStatusListener(JNIEnv *env, jobject obj) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        return adapter->removeAllStatusListener();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_isDeviceWhitelisted(JNIEnv *env, jobject obj, jstring jaddress) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        std::string saddress = from_jstring_to_string(env, jaddress);
        EUI48 address(saddress);
        return adapter->isDeviceWhitelisted(address);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}
jboolean Java_direct_1bt_tinyb_DBTAdapter_addDeviceToWhitelist__Ljava_lang_String_2IISSSS(JNIEnv *env, jobject obj,
                                                               jstring jaddress, int jaddressType, int jctype,
                                                               jshort min_interval, jshort max_interval,
                                                               jshort latency, jshort timeout) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        std::string saddress = from_jstring_to_string(env, jaddress);
        EUI48 address(saddress);
        const BDAddressType addressType = static_cast<BDAddressType>( jaddressType );
        const HCIWhitelistConnectType ctype = static_cast<HCIWhitelistConnectType>( jctype );
        return adapter->addDeviceToWhitelist(address, addressType, ctype, min_interval, max_interval, latency, timeout);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}
jboolean Java_direct_1bt_tinyb_DBTAdapter_addDeviceToWhitelist__Ljava_lang_String_2II(JNIEnv *env, jobject obj,
                                                               jstring jaddress, int jaddressType, int jctype) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        std::string saddress = from_jstring_to_string(env, jaddress);
        EUI48 address(saddress);
        const BDAddressType addressType = static_cast<BDAddressType>( jaddressType );
        const HCIWhitelistConnectType ctype = static_cast<HCIWhitelistConnectType>( jctype );
        return adapter->addDeviceToWhitelist(address, addressType, ctype);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}
jboolean Java_direct_1bt_tinyb_DBTAdapter_removeDeviceFromWhitelist(JNIEnv *env, jobject obj, jstring jaddress, int jaddressType) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);

        std::string saddress = from_jstring_to_string(env, jaddress);
        EUI48 address(saddress);
        const BDAddressType addressType = static_cast<BDAddressType>( jaddressType );
        return adapter->removeDeviceFromWhitelist(address, addressType);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jstring Java_direct_1bt_tinyb_DBTAdapter_toStringImpl(JNIEnv *env, jobject obj) {
    try {
        DBTAdapter *nativePtr = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(nativePtr->getJavaObject(), E_FILE_LINE);
        return from_string_to_jstring(env, nativePtr->toString());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

void Java_direct_1bt_tinyb_DBTAdapter_deleteImpl(JNIEnv *env, jobject obj, jlong nativeInstance)
{
    (void)obj;
    try {
        DBTAdapter *adapter = castInstance<DBTAdapter>(nativeInstance);
        DBG_PRINT("Java_direct_1bt_tinyb_DBTAdapter_deleteImpl %s", adapter->toString().c_str());
        delete adapter;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_isEnabled(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        return adapter->isEnabled();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_startDiscoveryImpl(JNIEnv *env, jobject obj, jboolean keepAlive)
{
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        return adapter->startDiscovery(keepAlive);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTAdapter_stopDiscoveryImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        return adapter->stopDiscovery();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jobject Java_direct_1bt_tinyb_DBTAdapter_getDiscoveredDevicesImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        std::vector<std::shared_ptr<DBTDevice>> array = adapter->getDiscoveredDevices();
        return convert_vector_sharedptr_to_jarraylist(env, array);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jint Java_direct_1bt_tinyb_DBTAdapter_removeDevicesImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        return adapter->removeDiscoveredDevices();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

//
// misc
//

void Java_direct_1bt_tinyb_DBTAdapter_setPowered(JNIEnv *env, jobject obj, jboolean value) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        adapter->setPowered(JNI_TRUE == value ? true : false);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jstring Java_direct_1bt_tinyb_DBTAdapter_getAlias(JNIEnv *env, jobject obj) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        return from_string_to_jstring(env, adapter->getLocalName().getName());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

void Java_direct_1bt_tinyb_DBTAdapter_setAlias(JNIEnv *env, jobject obj, jstring jnewalias) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        std::string newalias = from_jstring_to_string(env, jnewalias);
        adapter->setLocalName(newalias, std::string());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

void Java_direct_1bt_tinyb_DBTAdapter_setDiscoverable(JNIEnv *env, jobject obj, jboolean value) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        adapter->setDiscoverable(JNI_TRUE == value ? true : false);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jobject Java_direct_1bt_tinyb_DBTAdapter_connectDevice(JNIEnv *env, jobject obj, jstring jaddress, jstring jaddressType) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        std::string saddress = from_jstring_to_string(env, jaddress);
        EUI48 address(saddress);
        const BDAddressType addressType = fromJavaAdressTypeToBDAddressType(env, jaddressType);
        std::shared_ptr<DBTDevice> device = adapter->findDiscoveredDevice(address, addressType);
        if( nullptr != device ) {
            std::shared_ptr<direct_bt::HCIHandler> hci = adapter->getHCI();
            if( nullptr == hci ) {
                throw BluetoothException("Adapter's HCI not open "+adapter->toString(), E_FILE_LINE);
            }
            std::shared_ptr<JavaAnonObj> jDeviceRef = device->getJavaObject();
            JavaGlobalObj::check(jDeviceRef, E_FILE_LINE);

            device->connectDefault();
            return JavaGlobalObj::GetObject(jDeviceRef);
        }
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

void Java_direct_1bt_tinyb_DBTAdapter_setPairable(JNIEnv *env, jobject obj, jboolean value) {
    try {
        DBTAdapter *adapter = getDBTObject<DBTAdapter>(env, obj);
        JavaGlobalObj::check(adapter->getJavaObject(), E_FILE_LINE);
        adapter->setBondable(JNI_TRUE == value ? true : false);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

void Java_direct_1bt_tinyb_DBTAdapter_setDiscoveryFilter(JNIEnv *env, jobject obj, jobject juuids, jint rssi, jint pathloss, jint transportType) {
    // List<String> uuids
    (void)env;
    (void)obj;
    (void)juuids;
    (void)rssi;
    (void)pathloss;
    (void)transportType;
}

