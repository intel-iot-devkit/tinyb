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

#include "direct_bt_tinyb_DBTGattService.h"

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTDevice.hpp"
#include "direct_bt/DBTAdapter.hpp"

using namespace direct_bt;

jstring Java_direct_1bt_tinyb_DBTGattService_toStringImpl(JNIEnv *env, jobject obj) {
    try {
        GATTService *nativePtr = getInstance<GATTService>(env, obj);
        JavaGlobalObj::check(nativePtr->getJavaObject(), E_FILE_LINE);
        return from_string_to_jstring(env, nativePtr->toString());
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}


void Java_direct_1bt_tinyb_DBTGattService_deleteImpl(JNIEnv *env, jobject obj, jlong nativeInstance) {
    (void)obj;
    try {
        GATTService *service = castInstance<GATTService>(nativeInstance);
        (void)service;
        // No delete: Service instance owned by DBTDevice
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

static const std::string _characteristicClazzCtorArgs("(JLdirect_bt/tinyb/DBTGattService;S[Ljava/lang/String;Ljava/lang/String;SI)V");

jobject Java_direct_1bt_tinyb_DBTGattService_getCharacteristicsImpl(JNIEnv *env, jobject obj) {
    try {
        GATTService *service = getInstance<GATTService>(env, obj);
        JavaGlobalObj::check(service->getJavaObject(), E_FILE_LINE);

        std::vector<std::shared_ptr<GATTCharacteristic>> & characteristics = service->characteristicList;

        // DBTGattCharacteristic(final long nativeInstance, final DBTGattService service,
        //                       final short handle, final String[] properties,
        //                       final String value_type_uuid, final short value_handle,
        //                       final int clientCharacteristicsConfigIndex)

        std::function<jobject(JNIEnv*, jclass, jmethodID, GATTCharacteristic *)> ctor_char =
                [](JNIEnv *env, jclass clazz, jmethodID clazz_ctor, GATTCharacteristic *characteristic)->jobject {
                    // prepare adapter ctor
                    std::shared_ptr<GATTService> service = characteristic->getService();
                    if( nullptr == service ) {
                        throw IllegalStateException("Characteristic's GATTService destructed: "+characteristic->toString(), E_FILE_LINE);
                    }
                    JavaGlobalObj::check(service->getJavaObject(), E_FILE_LINE);
                    jobject jservice = JavaGlobalObj::GetObject(service->getJavaObject());

                    std::vector<std::unique_ptr<std::string>> props = GATTCharacteristic::getPropertiesStringList(characteristic->properties);
                    unsigned int props_size = props.size();

                    jclass string_class = search_class(env, "Ljava/lang/String;");
                    jobjectArray jproperties = env->NewObjectArray(props_size, string_class, 0);
                    java_exception_check_and_throw(env, E_FILE_LINE);

                    for (unsigned int i = 0; i < props_size; ++i) {
                        jobject elem = from_string_to_jstring(env, *props[i].get());
                        env->SetObjectArrayElement(jproperties, i, elem);
                    }
                    java_exception_check_and_throw(env, E_FILE_LINE);

                    const jstring uuid = from_string_to_jstring(env,
                            directBTJNISettings.getUnifyUUID128Bit() ? characteristic->value_type->toUUID128String() :
                                                                       characteristic->value_type->toString());
                    java_exception_check_and_throw(env, E_FILE_LINE);

                    jobject jchar = env->NewObject(clazz, clazz_ctor, (jlong)characteristic, jservice,
                            characteristic->handle, jproperties,
                            uuid, characteristic->value_handle, characteristic->clientCharacteristicsConfigIndex);
                    java_exception_check_and_throw(env, E_FILE_LINE);
                    JNIGlobalRef::check(jchar, E_FILE_LINE);
                    std::shared_ptr<JavaAnonObj> jCharRef = characteristic->getJavaObject();
                    JavaGlobalObj::check(jCharRef, E_FILE_LINE);

                    return JavaGlobalObj::GetObject(jCharRef);
                };
        return convert_vector_sharedptr_to_jarraylist<GATTCharacteristic>(env, characteristics, _characteristicClazzCtorArgs.c_str(), ctor_char);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

