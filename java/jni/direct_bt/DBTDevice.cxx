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
#include "direct_bt_tinyb_DBTAdapter.h"

#define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTDevice.hpp"
#include "direct_bt/DBTAdapter.hpp"

using namespace direct_bt;

void Java_direct_1bt_tinyb_DBTDevice_initImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

jboolean Java_direct_1bt_tinyb_DBTDevice_addStatusListener(JNIEnv *env, jobject obj, jobject statusListener)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);

        DBTAdapter * adapter = const_cast<DBTAdapter *>( &device->getAdapter() );
        std::shared_ptr<JavaAnonObj> adapterObjRef = adapter->getJavaObject();
        JavaGlobalObj::check(adapterObjRef, E_FILE_LINE);
        jobject jadapter = JavaGlobalObj::GetObject(adapterObjRef);

        return Java_direct_1bt_tinyb_DBTAdapter_addStatusListener(env, jadapter, statusListener);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

void Java_direct_1bt_tinyb_DBTDevice_deleteImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        delete device;
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

jboolean Java_direct_1bt_tinyb_DBTDevice_connectImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        uint16_t chandle = device->defaultConnect();
        if( 0 == chandle ) {
            return JNI_FALSE;
        }
        std::shared_ptr<GATTHandler> gatt = device->connectGATT();
        if( nullptr != gatt ) {
            // FIXME: Split up - may offload to other thread
            std::vector<GATTServiceDeclRef> & primServices = gatt->discoverCompletePrimaryServices();

            std::shared_ptr<GenericAccess> ga = gatt->getGenericAccess(primServices);
            if( nullptr != ga ) {
                env->SetShortField(obj, getField(env, obj, "appearance", "S"), static_cast<jshort>(ga->category));
                java_exception_check_and_throw(env, E_FILE_LINE);
                DBG_PRINT("GATT connected to GenericAccess: %s", ga->toString().c_str());
            }
#if 0
            std::shared_ptr<DeviceInformation> di = gatt->getDeviceInformation(primServices);
            if( nullptr != di ) {
                DBG_PRINT("GATT connected to DeviceInformation: %s", di->toString().c_str());
            }
#endif
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

jobject Java_direct_1bt_tinyb_DBTDevice_getServices(JNIEnv *env, jobject obj) {
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
        return nullptr;
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
