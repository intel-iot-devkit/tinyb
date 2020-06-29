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

#include "direct_bt_tinyb_DBTGattCharacteristic.h"

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTDevice.hpp"
#include "direct_bt/DBTAdapter.hpp"

using namespace direct_bt;

jstring Java_direct_1bt_tinyb_DBTGattCharacteristic_toStringImpl(JNIEnv *env, jobject obj) {
    try {
        GATTCharacteristic *nativePtr = getInstance<GATTCharacteristic>(env, obj);
        JavaGlobalObj::check(nativePtr->getJavaObject(), E_FILE_LINE);
        return from_string_to_jstring(env, nativePtr->toString());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

void Java_direct_1bt_tinyb_DBTGattCharacteristic_deleteImpl(JNIEnv *env, jobject obj) {
    try {
        GATTCharacteristic *characteristic = getInstance<GATTCharacteristic>(env, obj);
        (void)characteristic;
        // No delete: Service instance owned by GATTService -> DBTDevice
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

static const std::string _descriptorClazzCtorArgs("(JLdirect_bt/tinyb/DBTGattCharacteristic;Ljava/lang/String;S[B)V");

jobject Java_direct_1bt_tinyb_DBTGattCharacteristic_getDescriptorsImpl(JNIEnv *env, jobject obj) {
    try {
        GATTCharacteristic *characteristic = getInstance<GATTCharacteristic>(env, obj);
        JavaGlobalObj::check(characteristic->getJavaObject(), E_FILE_LINE);

        std::vector<GATTDescriptorRef> & descriptorList = characteristic->descriptorList;

        // DBTGattDescriptor(final long nativeInstance, final DBTGattCharacteristic characteristic,
        //                   final String type_uuid, final short handle, final byte[] value)

        // DBTGattDescriptor(final long nativeInstance, final DBTGattCharacteristic characteristic,
        //                   final String type_uuid, final short handle, final byte[] value)

        std::function<jobject(JNIEnv*, jclass, jmethodID, GATTDescriptor *)> ctor_desc =
                [](JNIEnv *env, jclass clazz, jmethodID clazz_ctor, GATTDescriptor *descriptor)->jobject {
                    // prepare adapter ctor
                    std::shared_ptr<GATTCharacteristic> characteristic = descriptor->getCharacteristic();
                    if( nullptr == characteristic ) {
                        throw IllegalStateException("Descriptor's GATTCharacteristic destructed: "+descriptor->toString(), E_FILE_LINE);
                    }
                    JavaGlobalObj::check(characteristic->getJavaObject(), E_FILE_LINE);
                    jobject jcharacteristic = JavaGlobalObj::GetObject(characteristic->getJavaObject());

                    const jstring uuid = from_string_to_jstring(env,
                            directBTJNISettings.getUnifyUUID128Bit() ? descriptor->type->toUUID128String() :
                                                                       descriptor->type->toString());
                    java_exception_check_and_throw(env, E_FILE_LINE);

                    const size_t value_size = descriptor->value.getSize();
                    jbyteArray jvalue = env->NewByteArray((jsize)value_size);
                    env->SetByteArrayRegion(jvalue, 0, (jsize)value_size, (const jbyte *)descriptor->value.get_ptr());
                    java_exception_check_and_throw(env, E_FILE_LINE);

                    jobject jchar = env->NewObject(clazz, clazz_ctor, (jlong)descriptor, jcharacteristic,
                            uuid, (jshort)descriptor->handle, jvalue);
                    java_exception_check_and_throw(env, E_FILE_LINE);
                    JNIGlobalRef::check(jchar, E_FILE_LINE);
                    std::shared_ptr<JavaAnonObj> jCharRef = descriptor->getJavaObject();
                    JavaGlobalObj::check(jCharRef, E_FILE_LINE);

                    return JavaGlobalObj::GetObject(jCharRef);
                };
        return convert_vector_sharedptr_to_jarraylist<GATTDescriptor>(env, descriptorList, _descriptorClazzCtorArgs.c_str(), ctor_desc);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jbyteArray Java_direct_1bt_tinyb_DBTGattCharacteristic_readValueImpl(JNIEnv *env, jobject obj) {
    try {
        GATTCharacteristic *characteristic = getInstance<GATTCharacteristic>(env, obj);
        JavaGlobalObj::check(characteristic->getJavaObject(), E_FILE_LINE);

        POctets res(GATTHandler::number(GATTHandler::Defaults::MAX_ATT_MTU), 0);
        if( !characteristic->readValue(res) ) {
            ERR_PRINT("Characteristic readValue failed: %s", characteristic->toString().c_str());
            return env->NewByteArray((jsize)0);
        }

        const size_t value_size = res.getSize();
        jbyteArray jres = env->NewByteArray((jsize)value_size);
        env->SetByteArrayRegion(jres, 0, (jsize)value_size, (const jbyte *)res.get_ptr());
        java_exception_check_and_throw(env, E_FILE_LINE);
        return jres;

    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

jboolean Java_direct_1bt_tinyb_DBTGattCharacteristic_writeValueImpl(JNIEnv *env, jobject obj, jbyteArray jvalue) {
    try {
        if( nullptr == jvalue ) {
            throw IllegalArgumentException("byte array null", E_FILE_LINE);
        }
        const int value_size = env->GetArrayLength(jvalue);
        if( 0 == value_size ) {
            return JNI_TRUE;
        }
        GATTCharacteristic *characteristic = getInstance<GATTCharacteristic>(env, obj);
        JavaGlobalObj::check(characteristic->getJavaObject(), E_FILE_LINE);

        JNICriticalArray<uint8_t> criticalArray(env); // RAII - release
        uint8_t * value_ptr = criticalArray.get(jvalue, criticalArray.Mode::NO_UPDATE_AND_RELEASE);
        if( NULL == value_ptr ) {
            throw InternalError("GetPrimitiveArrayCritical(byte array) is null", E_FILE_LINE);
        }
        TROOctets value(value_ptr, value_size);
        if( !characteristic->writeValue(value) ) {
            ERR_PRINT("Characteristic writeValue failed: %s", characteristic->toString().c_str());
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

jboolean Java_direct_1bt_tinyb_DBTGattCharacteristic_enableValueNotificationsImpl(JNIEnv *env, jobject obj, jboolean enable) {
    try {
        GATTCharacteristic *characteristic = getInstance<GATTCharacteristic>(env, obj);
        JavaGlobalObj::check(characteristic->getJavaObject(), E_FILE_LINE);

        bool cccdEnableResult[2];
        bool res = characteristic->configIndicationNotification(enable, enable, cccdEnableResult);
        DBG_PRINT("DBTGattCharacteristic::configIndicationNotification Config Notification(%d), Indication(%d): Result %d",
                cccdEnableResult[0], cccdEnableResult[1], res);
        (void) cccdEnableResult;
        return res;
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return JNI_FALSE;
}

