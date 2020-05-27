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

#include "direct_bt_tinyb_DBTDevice.h"

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTDevice.hpp"
#include "direct_bt/DBTAdapter.hpp"

using namespace direct_bt;

static const std::string _notificationReceivedMethodArgs("(Lorg/tinyb/BluetoothGattCharacteristic;[BJ)V");
static const std::string _indicationReceivedMethodArgs("(Lorg/tinyb/BluetoothGattCharacteristic;[BJZ)V");

class JNICharacteristicListener : public GATTCharacteristicListener {
  private:
    /**
        package org.tinyb;

        public abstract class GATTCharacteristicListener {
            long nativeInstance;

            public void notificationReceived(final BluetoothGattCharacteristic charDecl,
                                             final byte[] value, final long timestamp) {
            }

            public void indicationReceived(final BluetoothGattCharacteristic charDecl,
                                           final byte[] value, final long timestamp,
                                           final boolean confirmationSent) {
            }

        };
    */
    const GATTCharacteristic * characteristicMatchRef;
    std::shared_ptr<JavaAnonObj> deviceObjRef;
    std::unique_ptr<JNIGlobalRef> listenerObjRef;
    jmethodID  mNotificationReceived = nullptr;
    jmethodID  mIndicationReceived = nullptr;

  public:

    JNICharacteristicListener(JNIEnv *env, DBTDevice *device, jobject listener, const GATTCharacteristic * characteristicMatchRef) {
        deviceObjRef = device->getJavaObject();
        JavaGlobalObj::check(deviceObjRef, E_FILE_LINE);

        listenerObjRef = std::unique_ptr<JNIGlobalRef>(new JNIGlobalRef(listener));
        jclass listenerClazz = search_class(env, listenerObjRef->getObject());
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == listenerClazz ) {
            throw InternalError("CharacteristicListener not found", E_FILE_LINE);
        }

        this->characteristicMatchRef = characteristicMatchRef;

        mNotificationReceived = search_method(env, listenerClazz, "notificationReceived", _notificationReceivedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mNotificationReceived ) {
            throw InternalError("GATTCharacteristicListener has no notificationReceived"+_notificationReceivedMethodArgs+" method, for "+device->toString(), E_FILE_LINE);
        }
        mIndicationReceived = search_method(env, listenerClazz, "indicationReceived", _indicationReceivedMethodArgs.c_str(), false);
        java_exception_check_and_throw(env, E_FILE_LINE);
        if( nullptr == mNotificationReceived ) {
            throw InternalError("GATTCharacteristicListener has no indicationReceived"+_indicationReceivedMethodArgs+" method, for "+device->toString(), E_FILE_LINE);
        }
    }

    bool match(const GATTCharacteristic & characteristic) override {
        if( nullptr == characteristicMatchRef ) {
            return true;
        }
        return characteristic == *characteristicMatchRef;
    }

    void notificationReceived(GATTCharacteristicRef charDecl,
                              std::shared_ptr<TROOctets> charValue, const uint64_t timestamp) override {
        JNIEnv *env = *jni_env;
        try {
            JavaGlobalObj::check(charDecl->getJavaObject(), E_FILE_LINE);
            jobject jCharDecl = JavaGlobalObj::GetObject(charDecl->getJavaObject());

            const size_t value_size = charValue->getSize();
            jbyteArray jvalue = env->NewByteArray((jsize)value_size);
            env->SetByteArrayRegion(jvalue, 0, (jsize)value_size, (const jbyte *)charValue->get_ptr());
            java_exception_check_and_throw(env, E_FILE_LINE);


            env->CallVoidMethod(listenerObjRef->getObject(), mNotificationReceived,
                                jCharDecl, jvalue, (jlong)timestamp);
            java_exception_check_and_throw(env, E_FILE_LINE);
        } catch(...) {
            rethrow_and_raise_java_exception(env);
        }
    }

    void indicationReceived(GATTCharacteristicRef charDecl,
                            std::shared_ptr<TROOctets> charValue, const uint64_t timestamp,
                            const bool confirmationSent) override {
        JNIEnv *env = *jni_env;
        try {
            JavaGlobalObj::check(charDecl->getJavaObject(), E_FILE_LINE);
            jobject jCharDecl = JavaGlobalObj::GetObject(charDecl->getJavaObject());

            const size_t value_size = charValue->getSize();
            jbyteArray jvalue = env->NewByteArray((jsize)value_size);
            env->SetByteArrayRegion(jvalue, 0, (jsize)value_size, (const jbyte *)charValue->get_ptr());
            java_exception_check_and_throw(env, E_FILE_LINE);


            env->CallVoidMethod(listenerObjRef->getObject(), mIndicationReceived,
                                jCharDecl, jvalue, (jlong)timestamp, (jboolean)confirmationSent);
            java_exception_check_and_throw(env, E_FILE_LINE);
        } catch(...) {
            rethrow_and_raise_java_exception(env);
        }
    }
};


void Java_direct_1bt_tinyb_DBTDevice_initImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jstring Java_direct_1bt_tinyb_DBTDevice_toStringImpl(JNIEnv *env, jobject obj) {
    try {
        DBTDevice *nativePtr = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(nativePtr->getJavaObject(), E_FILE_LINE);
        return from_string_to_jstring(env, nativePtr->toString());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_addCharacteristicListener(JNIEnv *env, jobject obj, jobject listener, jobject jcharacteristicMatch) {
    try {
        if( nullptr == listener ) {
            throw IllegalArgumentException("characteristicListener is null", E_FILE_LINE);
        }
        {
            JNICharacteristicListener * pre =
                    getObjectRef<JNICharacteristicListener>(env, listener, "nativeInstance");
            if( nullptr != pre ) {
                WARN_PRINT("characteristicListener's nativeInstance not null, already in use");
                return false;
            }
        }
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
        if( nullptr == gatt ) {
            throw IllegalStateException("Characteristic's device GATTHandle not connected: "+ device->toString(), E_FILE_LINE);
        }

        GATTCharacteristic * characteristicMatchRef = nullptr;
        if( nullptr != jcharacteristicMatch ) {
            characteristicMatchRef = getInstance<GATTCharacteristic>(env, jcharacteristicMatch);
            JavaGlobalObj::check(characteristicMatchRef->getJavaObject(), E_FILE_LINE);
        }

        std::shared_ptr<GATTCharacteristicListener> l =
                std::shared_ptr<GATTCharacteristicListener>( new JNICharacteristicListener(env, device, listener, characteristicMatchRef) );

        if( gatt->addCharacteristicListener(l) ) {
            setInstance(env, listener, l.get());
            return JNI_TRUE;
        }
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_removeCharacteristicListener(JNIEnv *env, jobject obj, jobject statusListener) {
    try {
        if( nullptr == statusListener ) {
            throw IllegalArgumentException("characteristicListener is null", E_FILE_LINE);
        }
        JNICharacteristicListener * pre =
                getObjectRef<JNICharacteristicListener>(env, statusListener, "nativeInstance");
        if( nullptr == pre ) {
            WARN_PRINT("characteristicListener's nativeInstance is null, not in use");
            return false;
        }
        setObjectRef<JNICharacteristicListener>(env, statusListener, nullptr, "nativeInstance");

        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
        if( nullptr == gatt ) {
            throw IllegalStateException("Characteristic's device GATTHandle not connected: "+ device->toString(), E_FILE_LINE);
        }

        if( ! gatt->removeCharacteristicListener(pre) ) {
            WARN_PRINT("Failed to remove characteristicListener with nativeInstance: %p at %s", pre, device->toString().c_str());
            return false;
        }
        return true;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jint Java_direct_1bt_tinyb_DBTDevice_removeAllCharacteristicListener(JNIEnv *env, jobject obj) {
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
        if( nullptr == gatt ) {
            throw IllegalStateException("Characteristic's device GATTHandle not connected: "+ device->toString(), E_FILE_LINE);
        }

        return gatt->removeAllCharacteristicListener();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

void Java_direct_1bt_tinyb_DBTDevice_deleteImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        device->remove();
        // No delete: DBTDevice instance owned by DBTAdapter
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jboolean Java_direct_1bt_tinyb_DBTDevice_disconnectImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        device->disconnect();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_TRUE;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_remove(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        device->remove();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_TRUE;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_connectImpl__(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        uint16_t hciHandle = device->connectDefault();
        if( 0 == hciHandle ) {
            return JNI_FALSE;
        }
        std::shared_ptr<GATTHandler> gatt = device->connectGATT();
        if( nullptr != gatt ) {
            // FIXME: Split up - may offload to other thread
            std::vector<GATTServiceRef> services = device->getGATTServices(); // implicit GATT connect and discovery if required
            if( services.size() > 0 ) {
                std::shared_ptr<GenericAccess> ga = device->getGATTGenericAccess();
                if( nullptr != ga ) {
                    env->SetShortField(obj, getField(env, obj, "appearance", "S"), static_cast<jshort>(ga->appearance));
                    java_exception_check_and_throw(env, E_FILE_LINE);
                    DBG_PRINT("GATT connected to GenericAccess: %s", ga->toString().c_str());
                }
            }
            return JNI_TRUE;
        }
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_connectImpl__SSSSSS(JNIEnv *env, jobject obj,
                                                             jshort interval, jshort window,
                                                             jshort min_interval, jshort max_interval,
                                                             jshort latency, jshort timeout)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        uint16_t hciHandle;
        switch( device->addressType ) {
            case BDAddressType::BDADDR_LE_PUBLIC:
                hciHandle = device->connectLE(HCIAddressType::HCIADDR_LE_PUBLIC, HCIAddressType::HCIADDR_LE_PUBLIC,
                                              interval, window, min_interval, max_interval, latency, timeout);
                break;
            case BDAddressType::BDADDR_LE_RANDOM:
                hciHandle = device->connectLE(HCIAddressType::HCIADDR_LE_RANDOM, HCIAddressType::HCIADDR_LE_PUBLIC,
                                              interval, window, min_interval, max_interval, latency, timeout);
                break;
            default:
                hciHandle = device->connectDefault();
                break;
        }
        if( 0 == hciHandle ) {
            return JNI_FALSE;
        }
        std::shared_ptr<GATTHandler> gatt = device->connectGATT();
        if( nullptr != gatt ) {
            // FIXME: Split up - may offload to other thread
            std::vector<GATTServiceRef> services = device->getGATTServices(); // implicit GATT connect and discovery if required
            if( services.size() > 0 ) {
                std::shared_ptr<GenericAccess> ga = device->getGATTGenericAccess();
                if( nullptr != ga ) {
                    env->SetShortField(obj, getField(env, obj, "appearance", "S"), static_cast<jshort>(ga->appearance));
                    java_exception_check_and_throw(env, E_FILE_LINE);
                    DBG_PRINT("GATT connected to GenericAccess: %s", ga->toString().c_str());
                }
            }
            return JNI_TRUE;
        }
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

//
// getter
//

static const std::string _serviceClazzCtorArgs("(JLdirect_bt/tinyb/DBTDevice;ZLjava/lang/String;SS)V");

jobject Java_direct_1bt_tinyb_DBTDevice_getServices(JNIEnv *env, jobject obj) {
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);

        std::vector<GATTServiceRef> services = device->getGATTServices(); // implicit GATT connect and discovery if required

        // DBTGattService(final long nativeInstance, final DBTDevice device, final boolean isPrimary,
        //                final String type_uuid, final short handleStart, final short handleEnd)

        std::function<jobject(JNIEnv*, jclass, jmethodID, GATTService*)> ctor_service =
                [](JNIEnv *env, jclass clazz, jmethodID clazz_ctor, GATTService *service)->jobject {
                    // prepare adapter ctor
                    JavaGlobalObj::check(service->device->getJavaObject(), E_FILE_LINE);
                    jobject jdevice = JavaGlobalObj::GetObject(service->device->getJavaObject());
                    const jboolean isPrimary = service->isPrimary;
                    const jstring uuid = from_string_to_jstring(env,
                            directBTJNISettings.getUnifyUUID128Bit() ? service->type->toUUID128String() :
                                                                       service->type->toString());
                    java_exception_check_and_throw(env, E_FILE_LINE);

                    jobject jservice = env->NewObject(clazz, clazz_ctor, (jlong)service, jdevice, isPrimary,
                            uuid, service->startHandle, service->endHandle);
                    java_exception_check_and_throw(env, E_FILE_LINE);
                    JNIGlobalRef::check(jservice, E_FILE_LINE);
                    std::shared_ptr<JavaAnonObj> jServiceRef = service->getJavaObject();
                    JavaGlobalObj::check(jServiceRef, E_FILE_LINE);

                    return JavaGlobalObj::GetObject(jServiceRef);
                };
        return convert_vector_sharedptr_to_jarraylist<GATTService>(env, services, _serviceClazzCtorArgs.c_str(), ctor_service);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jshort Java_direct_1bt_tinyb_DBTDevice_getAppearance(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return (jshort) device->getAppearance();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return (jshort)0;
}

jstring Java_direct_1bt_tinyb_DBTDevice_getIcon(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return nullptr; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr; // FIXME;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_getPaired(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return JNI_FALSE; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_getTrusted(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return JNI_FALSE; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

void Java_direct_1bt_tinyb_DBTDevice_setTrusted(JNIEnv *env, jobject obj, jboolean value)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        (void)value;
        // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jboolean Java_direct_1bt_tinyb_DBTDevice_getBlocked(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return JNI_FALSE; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

void Java_direct_1bt_tinyb_DBTDevice_setBlocked(JNIEnv *env, jobject obj, jboolean value)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        (void)value;
        // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jboolean JNICALL Java_direct_1bt_tinyb_DBTDevice_getLegacyPairing(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return JNI_FALSE; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jshort Java_direct_1bt_tinyb_DBTDevice_getRSSI(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return (jshort) device->getRSSI();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}


jobjectArray Java_direct_1bt_tinyb_DBTDevice_getUUIDs(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return nullptr; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jstring Java_direct_1bt_tinyb_DBTDevice_getModalias(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return nullptr; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jobject Java_direct_1bt_tinyb_DBTDevice_getManufacturerData(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return nullptr; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jobject Java_direct_1bt_tinyb_DBTDevice_getServiceData(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return nullptr; // FIXME
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jshort Java_direct_1bt_tinyb_DBTDevice_getTxPower(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return (jshort) device->getTxPower();
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return 0;
}

#if 0
//
// Connected
//

static void disableConnectedNotifications(JNIEnv *env, jobject obj, DBTManager &mgmt)
{
    InvocationFunc<bool, std::shared_ptr<MgmtEvent>> * funcptr =
            getObjectRef<InvocationFunc<bool, std::shared_ptr<MgmtEvent>>>(env, obj, "connectedNotificationRef");
    if( nullptr != funcptr ) {
        FunctionDef<bool, std::shared_ptr<MgmtEvent>> funcDef( funcptr );
        funcptr = nullptr;
        setObjectRef(env, obj, funcptr, "connectedNotificationRef"); // clear java ref
        int count;
        if( 1 != ( count = mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, funcDef) ) ) {
            throw direct_bt::InternalError(std::string("removeMgmtEventCallback of ")+funcDef.toString()+" not 1 but "+std::to_string(count), E_FILE_LINE);
        }
    }
}
void Java_direct_1bt_tinyb_DBTDevice_disableConnectedNotificationsImpl(JNIEnv *env, jobject obj)
{
    // org.tinyb.BluetoothAdapterStatusListener
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        DBTManager & mgmt = device->getAdapter().getManager();

        disableConnectedNotifications(env, obj, mgmt);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}
void Java_direct_1bt_tinyb_DBTDevice_enableConnectedNotificationsImpl(JNIEnv *env, jobject obj, jobject javaCallback)
{
    // org.tinyb.BluetoothAdapterStatusListener
    try {
        DBTDevice *device= getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        DBTAdapter & adapter = device->getAdapter();
        DBTManager & mgmt = adapter.getManager();

        disableConnectedNotifications(env, obj, mgmt);

        bool(*nativeCallback)(JNIGlobalRef&, std::shared_ptr<MgmtEvent>) =
                [](JNIGlobalRef& javaCallback_ref, std::shared_ptr<MgmtEvent> e)->bool {
            const MgmtEvtConnected &event = *static_cast<const MgmtEvtConnected *>(e.get());

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
        mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceDisconnectedCB));
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

#endif
